import { Request, Response, NextFunction } from "express";
import { ClientService } from "../services/ClientService";
import { WebSocketService } from "../services/WebSockerServices";

let wsService: WebSocketService;
const clientService = new ClientService();

export const setWebSocketService = (service: WebSocketService) => {
  wsService = service;
};

export const getServerStatus = (req: Request, res: Response, next: NextFunction) => {
  const clientCount = wsService.getAllClients().size;
  res.status(200).json({
    status: "Server is running",
    connectedClients: clientCount
  });
};

export const testConnection = async (req: Request, res: Response, next: NextFunction): Promise<void> => {
  try {
    const { client_id } = req.params;

    const client = wsService.getClient(client_id);
    if (!clientService.validateClient(client)) {
      res.status(404).json({ message: "Client is not connected." });
      return
    }

    await clientService.sendCommand(client!, "test-connection", {});
    res.status(200).json({ message: "Test successful" });
  } catch (error) {
    next(error);
  }
};