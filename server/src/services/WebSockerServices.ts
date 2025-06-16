import { WebSocketServer, WebSocket as WsWebSocket } from "ws";
import { ExtendedWebSocket } from "../types";
import logger from "../utils/logger";

export class WebSocketService {
  private wss: WebSocketServer
  private connectedClients: Map<string, ExtendedWebSocket>;
  private heartbeatInterval: NodeJS.Timeout | null = null;
  private clientTimeout: number;

  constructor() {
    this.wss = new WebSocketServer({ port: 8080 });
    this.connectedClients = new Map<string, ExtendedWebSocket>();
    this.clientTimeout = 15000;
    this.setupWebSocketServer();
  }

  setupWebSocketServer() {
    this.wss.on("connection", (ws: ExtendedWebSocket) => {
      ws.isAlive = true;
      ws.lastHeartBeat = Date.now();

      ws.on("pong", () => {
        ws.isAlive = true;
        ws.lastHeartBeat = Date.now();
      });

      ws.on("message", this.handleMessage(ws));
      ws.on("close", () => this.handleClose(ws));
      ws.on("error", (error: Error) => this.handleError(ws, error));
    })

    this.startHeartbeat();
  }

  getClient(clientId: string): ExtendedWebSocket | undefined {
    return this.connectedClients.get(clientId);
  }

  getAllClients(): Map<string, ExtendedWebSocket> {
    return this.connectedClients;
  }

  closeAllConnections() {
    this.wss.clients.forEach(client => {
      client.terminate();
    });
    this.connectedClients.clear();
  }

  private handleMessage(ws: ExtendedWebSocket) {
    return (msg: Buffer) => {
      const message = msg.toString();
      let client;

      try {
        const data = JSON.parse(message);

        // Handle heartbeat from client
        if (data.type === 'heartbeat') {
          ws.isAlive = true;
          ws.lastHeartBeat = Date.now();
          ws.send(JSON.stringify({ type: 'heartbeat_ack', timeStamp: Date.now() }));
          return;
        }

        if (data.nome) {
          if (this.connectedClients.has(data.nome)) {
            logger.warn("Client", `Client ${data.nome} already connected...`);
          } else {
            ws.id = data.nome;
            ws.requestStatus = false;
            logger.info("Client", `New client connected: ${ws.id}`);
            this.connectedClients.set(data.nome, ws);
          }
        }
        client = this.getClient(data.id);

        if (data.status === "ok") {
          if (ws.id && this.connectedClients.has(ws.id)) {
            // TODO: If not found client, take some action
            if (client) {
              client.requestStatus = true;
              logger.info("Client", `${ws.id}: Connection established!`)
            }
          }
        }

        //! Melhorar tratamento de resposta do cliente e fazer respostas personalizadas
        // Handles client's request answer
        if (data.callBack && client) {
          logger.info("Client", `Response from client: ${data.callBack.client} for command: ${data?.callBack?.command}`)

          client.requestStatus = true;
        }
      } catch (error) {
        logger.error("WebSocketService", `Error parsing message: ${error}`);
      }
    }
  }

  private handleClose(ws: ExtendedWebSocket) {
    if (ws.id) {
      this.connectedClients.delete(ws.id);
      logger.info("Client", `Client disconnected: ${ws.id}`);
    }
  }

  private handleError(ws: ExtendedWebSocket, error: Error) {
    logger.error("Client", `WebSocket error for ${ws.id}: ${error.message}`);
    if (ws.id) {
      this.connectedClients.delete(ws.id);
    }
  }

  private startHeartbeat() {
    this.heartbeatInterval = setInterval(() => {
      const now = Date.now();

      this.wss.clients.forEach((client) => {
        const ws = client as ExtendedWebSocket;

        if (now - ws.lastHeartBeat > this.clientTimeout) {
          if (ws.id) {
            this.connectedClients.delete(ws.id);
            logger.info("Client", `Client removed because of timeout: ${ws.id}`);
          }

          ws.terminate();
          return;
        }

        if (!ws.isAlive) {
          if (ws.id) {
            this.connectedClients.delete(ws.id);
            logger.info("Client", `Client removed because of inactivity: ${ws.id}`);
          }
          ws.terminate();
          return;
        }

        ws.isAlive = false;
        ws.ping();
      });
    }, this.clientTimeout);

    this.wss.on("close", () => {
      if (this.heartbeatInterval) {
        clearInterval(this.heartbeatInterval);
      }
    });
  }
}
