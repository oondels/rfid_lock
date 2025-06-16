import express from "express";
import * as rfidController from "../controllers/rfidController";

export const rfidRoute = express.Router();

rfidRoute.get("/get_all/:client_id", rfidController.getAllRFIDs);
rfidRoute.post("/add/:client_id", rfidController.addRFIDs);
rfidRoute.post("/remove/:client_id", rfidController.removeRFID);
rfidRoute.post("/clear/:client_id", rfidController.clearRFIDs);