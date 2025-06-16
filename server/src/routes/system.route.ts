import express from "express";
import * as systemController from "../controllers/systemController";

export const sytemRoute = express.Router();

sytemRoute.get("/test-connection/:client_id", systemController.testConnection);
sytemRoute.get("/", systemController.getServerStatus);
