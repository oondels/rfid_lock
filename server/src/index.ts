import express, { Request, Response, NextFunction, RequestHandler } from "express";
import http from "http";
import cors from "cors";
import helmet from "helmet";
import logger from "./utils/logger";
import dotenv from "dotenv";
import { WebSocketServer, WebSocket as WsWebSocket } from "ws";
dotenv.config();

interface ExtendedWebSocket extends WsWebSocket {
  id?: string;
  requestStatus?: boolean;
  isAlive?: boolean;
}

const app = express();
const port = 3010;
const server = http.createServer(app);
const wss = new WebSocketServer({ server });

app.use(helmet());
app.use(cors());
app.use(express.json());

let connectedClients = new Map<string, ExtendedWebSocket>();
wss.on("connection", (ws: ExtendedWebSocket) => {
  ws.isAlive = true;

  ws.on("pong", () => {
    ws.isAlive = true;
  });

  ws.on("message", (msg: Buffer) => {
    const message = msg.toString();
    let client;

    try {
      const data = JSON.parse(message);


      //! Verificar bug de client estar conectado mas nao disponivel para receber mensagens
      if (data.nome) {
        if (connectedClients.has(data.nome)) {
          logger.warn("Client", `Client ${data.nome} already connected...`);
        } else {
          ws.id = data.nome;
          ws.requestStatus = false;
          logger.info("Client", `New client connected: ${ws.id}`);
          connectedClients.set(data.nome, ws);
        }

        // console.log("Clients: ", connectedClients.keys());
        // console.log("Clients count: ", connectedClients.size);

      }
      client = connectedClients.get(ws.id as string);

      if (data.status === 'ok') {
        if (ws.id && connectedClients.has(ws.id)) {
          // TODO: If not found client, take some action
          if (client) {
            client.requestStatus = true;
            logger.info("Client", `${ws.id}: Connection established!`)
          }
        }
      }

      // Handles client's request answer
      //! Melhorar tratamento de resposta do cliente e fazer respostas personalizadas
      if (data.callBack && client) {
        logger.info("Client", `Response from client: ${data.callBack.client} for command: ${data?.callBack?.command}`)

        client.requestStatus = true;
      }

      console.log(data);
    } catch (e) {
      console.error("Error parsing client message!", e);
    }
  });

  ws.on("close", () => {
    if (ws.id) {
      connectedClients.delete(ws.id);
      logger.info("Client", `Client disconnected: ${ws.id}`);
    }
  });
});

const heartBeatInterval = 20000;
const interval = setInterval(() => {
  wss.clients.forEach((client) => {
    const ws = client as ExtendedWebSocket;

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
const sendCommand = (client: ExtendedWebSocket, command: string, payload: object): Promise<void> => {
  return new Promise((resolve, reject) => {
    client.requestStatus = false;
    client.send(JSON.stringify({ command, ...payload, }));

    const timer = setTimeout(() => {
      reject(new Error("Timeout waiting client response!"));
    }, 3000);

    const checkResponse = setInterval(() => {
      if (client.requestStatus) {
        clearInterval(checkResponse);
        clearTimeout(timer);
        resolve();
      }
    }, 100);
  });
};

const getAllRFIDHandler: RequestHandler = async (req: Request, res: Response, next: NextFunction) => {
  try {
    const { client_id } = req.params;

    const client = connectedClients.get(client_id);
    if (!client || client.readyState !== WebSocket.OPEN) {
      res.status(404).json({ message: "Client is not connected." });
      return;
    }

    await sendCommand(client, "get_all", { client: client_id });

    res.status(200).json({ message: "RFIDs retrieved successfully." });
  } catch (error) {
    next(error);
  }
};
app.get("/api/get_all/:client_id", getAllRFIDHandler);

const addRFIDHandler: RequestHandler = async (req: Request, res: Response, next: NextFunction) => {
  try {
    const { rfids } = req.body;
    const { client_id } = req.params;

    if (!rfids || !Array.isArray(rfids)) {
      res.status(400).json({ message: "Invalid rfids." });
      return;
    }

    const client = connectedClients.get(client_id);
    if (!client || client.readyState !== WebSocket.OPEN) {
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
    if (!client || client.readyState !== WebSocket.OPEN) {
      res.status(404).json({ message: "Client is not connected." });
      return;
    }

    await sendCommand(client, "remove_rfid", { rfid });

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
    if (!client || client.readyState !== WebSocket.OPEN) {
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
    if (!client || client.readyState !== WebSocket.OPEN) {
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

    if (verificationKey !== process.env.VERIFICATION_KEY) {
      res.status(403).json({ message: "Invalid verification key." });
      return;
    }

    const client = connectedClients.get(client_id);
    if (!client || client.readyState !== WebSocket.OPEN) {
      res.status(404).json({ message: "Client is not connected." });
      return;
    }

    await sendCommand(client, "open_door", {});

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
