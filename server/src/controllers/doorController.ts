import { Request, Response, NextFunction } from "express";
import { ClientService } from "../services/ClientService";
import { WebSocketService } from "../services/WebSockerServices";
// import config from "../config/config";

let wsService: WebSocketService;
const clientService = new ClientService();

export const setWebSocketService = (service: WebSocketService) => {
  wsService = service;
};

export const openDoor = async (req: Request, res: Response, next: NextFunction): Promise<void> => {
  try {
    const { client_id } = req.params;
    const { verificationKey } = req.body;


    //TODO: Replace 'gegeg' with the actual verification key from your config or environment variable
    if (verificationKey !== 'gegeg') {
      res.status(403).json({ message: "Invalid verification key." });
      return
    }

    const client = wsService.getClient(client_id);
    if (!clientService.validateClient(client)) {
      res.status(404).json({ message: "Client is not connected." });
      return
    }

    await clientService.sendCommand(client!, "open_door", {});
    res.status(201).json({ message: "Lock opened successfully." });
  } catch (error) {
    next(error);
  }
};

export const getAccessHistory = async (req: Request, res: Response, next: NextFunction): Promise<void> => {
  try {
    const { client_id } = req.params;

    const client = wsService.getClient(client_id);
    if (!clientService.validateClient(client)) {
      res.status(404).json({ message: "Client is not connected." });
      return
    }

    await clientService.sendCommand(client!, "get_access_history", { client: client_id });
    res.status(200).json({ message: "Last Access retrieved successfully." });
  } catch (error) {
    next(error);
  }
};

export const clearAccessHistory = async (req: Request, res: Response, next: NextFunction): Promise<void> => {
  try {
    const { client_id } = req.params;

    const client = wsService.getClient(client_id);
    if (!clientService.validateClient(client)) {
      res.status(404).json({ message: "Client is not connected." });
      return
    }

    await clientService.sendCommand(client!, "clear_history", { client: client_id });
    res.status(200).json({ message: "History Cleared" });
  } catch (error) {
    next(error);
  }
};