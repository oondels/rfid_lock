import express, { Request, Response, NextFunction } from "express";
import http from "http";
import cors from "cors";
import helmet from "helmet";
import logger from "./utils/logger";
import dotenv from "dotenv";
import { rfidRoute } from "./routes/rfid.route";
import { doorRoute } from "./routes/door.route";
import { sytemRoute } from "./routes/system.route";
dotenv.config();

const app = express();
const port = 3010;
const server = http.createServer(app);


app.use(helmet());
app.use(cors());
app.use(express.json());

app.use("/api/rfid", rfidRoute);
app.use("/api/door", doorRoute);
app.use("/api/system", sytemRoute);

app.use((error: Error, req: Request, res: Response, next: NextFunction) => {
  logger.error("Server", `Error on ${req.method} ${req.originalUrl}: ${error.message}`);
  res.status(500).json({ error: "Internal Server Error." });
});

server.listen(port, () => {
  logger.info("Server", `Server running on port: ${port}`);
});
