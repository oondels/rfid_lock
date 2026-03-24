import express, { Request, Response, NextFunction, RequestHandler } from "express";
import http from "http";
import cors from "cors";
import helmet from "helmet";
import logger from "./utils/logger";
import dotenv from "dotenv";
import { WebSocketServer, WebSocket as WsWebSocket } from "ws";
import { randomUUID } from "crypto";
dotenv.config();

interface ExtendedWebSocket extends WsWebSocket {
  id?: string;
  isAlive?: boolean;
  lastHeartBeat: number;
}

type PendingRequest = {
  resolve: (data: Record<string, unknown>) => void;
  reject: (error: Error) => void;
  timer: NodeJS.Timeout;
  command: string;
};

const app = express();
const port = 3010;
const server = http.createServer(app);

const wss = new WebSocketServer({ server });

app.use(helmet());
app.use(cors());
app.use(express.json());

let connectedClients = new Map<string, ExtendedWebSocket>();
const pendingRequests = new Map<string, PendingRequest>();
const heartBeatInterval = 10000;
const clientTimeout = 30000;

const pendingKey = (clientId: string, requestId: string): string => `${clientId}:${requestId}`;

const clearPendingRequestsForClient = (clientId: string, reason: string): void => {
  for (const [key, request] of pendingRequests) {
    if (!key.startsWith(`${clientId}:`)) {
      continue;
    }

    clearTimeout(request.timer);
    request.reject(new Error(reason));
    pendingRequests.delete(key);
  }
};

wss.on("connection", (ws: ExtendedWebSocket) => {
  ws.isAlive = true;
  ws.lastHeartBeat = Date.now();

  ws.on("pong", () => {
    ws.isAlive = true;
    ws.lastHeartBeat = Date.now();
  });

  ws.on("message", (msg: Buffer) => {
    const message = msg.toString();
    let client;

    try {
      const data = JSON.parse(message);
      ws.isAlive = true;
      ws.lastHeartBeat = Date.now();

      // Handle heartbeat from client
      if (data.type === 'heartbeat') {
        ws.send(JSON.stringify({ type: 'heartbeat_ack', timeStamp: Date.now() }))
        return;
      }

      if (data.nome) {
        const existingClient = connectedClients.get(data.nome);
        if (existingClient && existingClient !== ws) {
          logger.warn("Client", `Replacing existing connection for client ${data.nome}`);
          clearPendingRequestsForClient(data.nome, "Client connection replaced.");
          existingClient.terminate();
        }

        ws.id = data.nome;
        connectedClients.set(data.nome, ws);
        logger.info("Client", `Client registered: ${ws.id}`);
      }
      client = ws.id ? connectedClients.get(ws.id) : undefined;

      if (data.status === 'ok') {
        if (ws.id && connectedClients.has(ws.id)) {
          logger.info("Client", `${ws.id}: Connection established!`)
        }
      }

      // Handles client's request answer
      if (data.callBack && client && ws.id) {
        const requestId = data?.callBack?.requestId;
        const command = data?.callBack?.command;
        const status = data?.callBack?.status;

        if (!requestId || typeof requestId !== "string") {
          logger.warn("Client", `Ignoring callback without requestId from ${ws.id}`);
          return;
        }

        const key = pendingKey(ws.id, requestId);
        const pendingRequest = pendingRequests.get(key);
        if (!pendingRequest) {
          logger.warn("Client", `No pending request found for ${ws.id} and requestId ${requestId}`);
          return;
        }

        clearTimeout(pendingRequest.timer);
        pendingRequests.delete(key);

        logger.info("Client", `Response from client: ${ws.id} for command: ${command}`)
        if (status === "error") {
          const errorMsg = data?.error || "Client returned error status.";
          pendingRequest.reject(new Error(errorMsg));
          return;
        }

        pendingRequest.resolve(data.callBack as Record<string, unknown>);
      }
    } catch (e) {
      console.error("Error parsing client message!", e);
    }
  });

  ws.on("close", () => {
    if (ws.id) {
      clearPendingRequestsForClient(ws.id, "Client disconnected.");
      connectedClients.delete(ws.id);
      logger.info("Client", `Client disconnected: ${ws.id}`);
    }
  });

  ws.on("error", (error) => {
    logger.error("Client", `WebSocket error for ${ws.id}: ${error.message}`);
    if (ws.id) {
      clearPendingRequestsForClient(ws.id, "Client connection error.");
      connectedClients.delete(ws.id);
    }
  });
});

const interval = setInterval(() => {
  const now = Date.now();

  wss.clients.forEach((client) => {
    const ws = client as ExtendedWebSocket;

    if (now - ws.lastHeartBeat > clientTimeout) {
      if (ws.id) {
        connectedClients.delete(ws.id);
        logger.info("Client", `Client removed because of timeout: ${ws.id}`);
      }

      ws.terminate();
      return;
    }

    if (!ws.isAlive) {
      if (ws.id) {
        connectedClients.delete(ws.id);
        logger.info("Client", `Client removed because of inactivity: ${ws.id}`);
      }
      ws.terminate();
      return;
    }

    ws.isAlive = false;
    ws.ping();
  });
}, heartBeatInterval);

wss.on("close", () => {
  clearInterval(interval);
});

app.get("/", (req: Request, res: Response) => {
  res.send("Server is running.");
});

// Send a especific command to a client, and wait for asnwer
const sendCommand = (client: ExtendedWebSocket, command: string, payload: object): Promise<Record<string, unknown>> => {
  return new Promise((resolve, reject) => {
    if (client.readyState !== WsWebSocket.OPEN) {
      reject(new Error("Client is not connected."));
      return;
    }

    if (!client.id) {
      reject(new Error("Client is not registered."));
      return;
    }

    const requestId = randomUUID();
    const key = pendingKey(client.id, requestId);
    const timer = setTimeout(() => {
      pendingRequests.delete(key);
      reject(new Error(`Timeout waiting response for command ${command}.`));
    }, 3000);

    pendingRequests.set(key, {
      resolve,
      reject,
      timer,
      command,
    });

    try {
      client.send(JSON.stringify({ command, requestId, ...payload }));
    } catch (error) {
      clearTimeout(timer);
      pendingRequests.delete(key);
      reject(error as Error);
    }
  });
};

const getAllRFIDHandler: RequestHandler = async (req: Request, res: Response, next: NextFunction) => {
  try {
    const { client_id } = req.params;

    const client = connectedClients.get(client_id);
    if (!client || client.readyState !== WsWebSocket.OPEN) {
      res.status(404).json({ message: "Client is not connected." });
      return;
    }

    const result = await sendCommand(client, "get_all", { client: client_id });

    res.status(200).json({ rfids: result?.rfids_list ?? [] });
  } catch (error) {
    next(error);
  }
};
app.get("/api/get_all/:client_id", getAllRFIDHandler);

const getLastAccess: RequestHandler = async (req: Request, res: Response, next: NextFunction) => {
  try {
    const { client_id } = req.params;

    console.log(`Getting last access for client: ${client_id}`);

    const client = connectedClients.get(client_id);
    if (!client || client.readyState !== WsWebSocket.OPEN) {
      res.status(404).json({ message: "Client is not connected." });
      return;
    }

    const result = await sendCommand(client, "get_access_history", { client: client_id });

    res.status(200).json({
      access_history: result?.access_history ?? [],
      last_accessed_card: result?.last_accessed_card ?? null,
    });
  } catch (error) {
    next(error);
  }
};
app.get("/api/get_access_history/:client_id", getLastAccess);

const clearHistory: RequestHandler = async (req: Request, res: Response, next: NextFunction) => {
  try {
    const { client_id } = req.params;

    console.log(`Clearing history for client: ${client_id}`);


    const client = connectedClients.get(client_id);
    if (!client || client.readyState !== WsWebSocket.OPEN) {
      res.status(404).json({ message: "Client is not connected." });
      return;
    }
    await sendCommand(client, "clear_history", { client: client_id });

    res.status(200).json({ message: "History Cleared" });
  } catch (error) {
    next(error);
  }
};
app.get("/api/clear_history/:client_id", clearHistory);
app.post("/api/clear_history/:client_id", clearHistory);

const addRFIDHandler: RequestHandler = async (req: Request, res: Response, next: NextFunction) => {
  try {
    const { rfids } = req.body;
    const { client_id } = req.params;

    if (!rfids || !Array.isArray(rfids)) {
      res.status(400).json({ message: "Invalid rfids." });
      return;
    }

    const client = connectedClients.get(client_id);
    if (!client || client.readyState !== WsWebSocket.OPEN) {
      res.status(404).json({ message: "Client is not connected." });
      return;
    }

    await sendCommand(client, "add_rfids", { rfids, client: client_id });

    res.status(201).json({ message: "RFIDs added successfully." });
  } catch (error) {
    next(error);
  }
};
app.post("/api/add/:client_id", addRFIDHandler);

const removeRFIDHandler: RequestHandler = async (req: Request, res: Response, next: NextFunction) => {
  try {
    const { rfid } = req.body;
    const { client_id } = req.params;

    if (!rfid) {
      res.status(400).json({ message: "Invalid rfid." });
      return;
    }

    const client = connectedClients.get(client_id);
    if (!client || client.readyState !== WsWebSocket.OPEN) {
      res.status(404).json({ message: "Client is not connected." });
      return;
    }

    await sendCommand(client, "remove_rfid", { rfid, client: client_id });

    res.status(201).json({ message: "RFID removed successfully." });
  } catch (error) {
    next(error);
  }
};
app.post("/api/remove/:client_id", removeRFIDHandler);

const clearRfidsHandler: RequestHandler = async (req: Request, res: Response, next: NextFunction) => {
  try {
    const { client_id } = req.params;

    const client = connectedClients.get(client_id);
    if (!client || client.readyState !== WsWebSocket.OPEN) {
      res.status(404).json({ message: "Client is not connected." });
      return;
    }

    await sendCommand(client, "clear-list", {});

    res.status(201).json({ message: "RFIDs cleared successfully." });
  } catch (error) {
    next(error);
  }
};
app.post("/api/clear/:client_id", clearRfidsHandler);

const testConnectionHandler: RequestHandler = async (req: Request, res: Response, next: NextFunction) => {
  try {
    const { client_id } = req.params;

    const client = connectedClients.get(client_id);
    if (!client || client.readyState !== WsWebSocket.OPEN) {
      res.status(404).json({ message: "Client is not connected." });
      return;
    }

    await sendCommand(client, "test-connection", {});

    res.status(200).json({ message: "Test successful" });
  } catch (error) {
    next(error);
  }
};
app.get("/api/test-connection/:client_id", testConnectionHandler);

const openLockHandler: RequestHandler = async (req: Request, res: Response, next: NextFunction) => {
  try {
    const { client_id } = req.params;
    const { verificationKey } = req.body;

    if (!process.env.VERIFICATION_KEY) {
      logger.warn("Server", "VERIFICATION_KEY is not configured. Open lock endpoint is using fallback mode.");
    } else {
      if (!verificationKey || typeof verificationKey !== "string") {
        res.status(400).json({ message: "verificationKey is required." });
        return;
      }

      if (verificationKey !== process.env.VERIFICATION_KEY) {
        res.status(403).json({ message: "Invalid verification key." });
        return;
      }
    }

    const client = connectedClients.get(client_id);
    if (!client || client.readyState !== WsWebSocket.OPEN) {
      res.status(404).json({ message: "Client is not connected." });
      return;
    }

    await sendCommand(client, "open_door", { client: client_id });

    res.status(201).json({ message: "Lock opened successfully." });
  } catch (error) {
    next(error);
  }
};
app.post("/api/open/:client_id", openLockHandler);

app.use((error: Error, req: Request, res: Response, next: NextFunction) => {
  logger.error("Server", `Error on ${req.method} ${req.originalUrl}: ${error.message}`);
  res.status(500).json({ error: "Internal Server Error." });
});

server.listen(port, () => {
  console.log("Server running on port: ", port);
});
