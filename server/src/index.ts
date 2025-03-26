import express, { Request, Response, NextFunction } from "express";
import cors from "cors";
import http from "http";
import { WebSocketServer, WebSocket } from "ws";

declare module "ws" {
  interface WebSocket {
    id?: string;
    requestStatus?: boolean;
  }
}

const app = express();
const port = 3010;
const server = http.createServer(app);
const wss = new WebSocketServer({ server });

app.use(cors());
app.use(express.json());

let connectedClients = new Map<string, WebSocket>();
wss.on("connection", (ws: WebSocket) => {

  ws.on("message", (msg: Buffer) => {
    const message = msg.toString();

    try {
      const data = JSON.parse(message);

      if (data.nome) {
        if (connectedClients.has(data.nome)) {
          console.warn(`Client ${data.nome} already connected...`);
        }

        ws.id = data.nome;
        ws.requestStatus = false;
        console.log("New client connected: ", ws.id);
        connectedClients.set(data.nome, ws);
      }
    } catch (e) {
      if (message === "confirmar") {
        if (ws.id && connectedClients.has(ws.id)) {
          const client = connectedClients.get(ws.id);

          if (client) {
            client.requestStatus = true;
          }
        }
      }
    }
  });

  ws.on("close", () => {
    connectedClients.delete(ws.id);
    console.log("Client disconnected: ", ws.id);
  });
});

app.get("/", (req: Request, res: Response) => {
  res.send("Server is running.");
});

app.post("/api/add/:client_id", (req: Request, res: Response, next: NextFunction) => {
  try {
    const { rfids } = req.body;
    const { client_id } = req.params;

    if (!rfids || !Array.isArray(rfids)) {
      return res.status(400).json({ message: "Invalid rfids." });
    }

    const client: WebSocket = connectedClients.get(client_id);
    if (!client || client.readyState !== WebSocket.OPEN) {
      return res.status(404).json({ message: "Client is not connected." });
    }

    client.send(
      JSON.stringify(
        {
          command: "adicionar",
          rfids: rfids
        }
      )
    )

    setTimeout(() => {
      if (client.requestStatus) {
        res.status(201).json({ message: "RFIDs added successfully." });
      } else {
        res.status(500).json({ message: "Failed to add RFIDs." });
      }
    }, 3000);
  } catch (error) {
    next(error);
  }
});

app.use((error: Error, req: Request, res: Response, next: NextFunction) => {
  console.error(`Erro on ${req.method} ${req.originalUrl}: ${error.message}`);
  res.status(500).json({ error: "Internal Server Error." });
});

server.listen(port, () => {
  console.log("Server running on port: ", port);
});
