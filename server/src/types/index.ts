import { WebSocket } from "ws";

export interface ExtendedWebSocket extends WebSocket {
  id?: string;
  requestStatus?: boolean;
  isAlive?: boolean;
  lastHeartBeat: number;
}

export interface CommandPayload {
  [key: string]: any;
}