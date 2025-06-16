import { Request, Response, NextFunction } from "express";
import { ClientService } from "../services/ClientService";
import { WebSocketService } from "../services/WebSockerServices";

let wsService: WebSocketService;
const clientService = new ClientService();

export const setWebSocketService = (service: WebSocketService) => {
  wsService = service;
};

export const getAllRFIDs = async (req: Request, res: Response, next: NextFunction): Promise<void> => {
  try {
    const { client_id } = req.params;

    const client = wsService.getClient(client_id);
    if (!clientService.validateClient(client)) {
      res.status(404).json({ message: "Client is not connected." });
      return;
    }

    await clientService.sendCommand(client!, "get_all", { client: client_id });
    res.status(200).json({ message: "RFIDs retrieved successfully." });
  } catch (error) {
    next(error);
  }
};

export const addRFIDs = async (req: Request, res: Response, next: NextFunction): Promise<void> => {
  try {
    const { rfids } = req.body;
    const { client_id } = req.params;

    if (!rfids || !Array.isArray(rfids)) {
      res.status(400).json({ message: "Invalid rfids." });
      return;
    }

    const client = wsService.getClient(client_id);
    if (!clientService.validateClient(client)) {
      res.status(404).json({ message: "Client is not connected." });
      return
    }

    await clientService.sendCommand(client!, "add_rfids", { rfids, client: client_id });
    res.status(201).json({ message: "RFIDs added successfully." });
  } catch (error) {
    next(error);
  }
};

export const removeRFID = async (req: Request, res: Response, next: NextFunction): Promise<void> => {
  try {
    const { rfid } = req.body;
    const { client_id } = req.params;

    if (!rfid) {
       res.status(400).json({ message: "Invalid rfid." });
       return
    }

    const client = wsService.getClient(client_id);
    if (!clientService.validateClient(client)) {
      res.status(404).json({ message: "Client is not connected." });
      return
    }

    await clientService.sendCommand(client!, "remove_rfid", { rfid });
    res.status(201).json({ message: "RFID removed successfully." });
  } catch (error) {
    next(error);
  }
};

export const clearRFIDs = async (req: Request, res: Response, next: NextFunction): Promise<void> => {
  try {
    const { client_id } = req.params;

    const client = wsService.getClient(client_id);
    if (!clientService.validateClient(client)) {
      res.status(404).json({ message: "Client is not connected." });
      return
    }

    await clientService.sendCommand(client!, "clear-list", {});
    res.status(201).json({ message: "RFIDs cleared successfully." });
  } catch (error) {
    next(error);
  }
};