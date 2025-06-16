import { ExtendedWebSocket, CommandPayload } from "../types";
import logger from "../utils/logger";
// import config from "../config/config";

export class ClientService {
  async sendCommand(client: ExtendedWebSocket, command: string, payload: CommandPayload): Promise<void> {
    return new Promise((resolve, reject) => {
      client.requestStatus = false;
      logger.debug("Command", `Sending command ${command} to client ${client.id}`);

      client.send(JSON.stringify({ command, ...payload }));

      const timer = setTimeout(() => {
        reject(new Error("Timeout waiting client response!"));
      }, 4000);

      const checkResponse = setInterval(() => {
        if (client.requestStatus) {
          clearInterval(checkResponse);
          clearTimeout(timer);
          resolve();
        }
      }, 100);
    });
  }

  validateClient(client?: ExtendedWebSocket): boolean {
    return !!client && client.readyState === 1; // 1 = WebSocket.OPEN
  }
}