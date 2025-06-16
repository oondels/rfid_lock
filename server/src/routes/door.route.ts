import express from "express";
import * as doorController from "../controllers/doorController";

export const doorRoute = express.Router();

doorRoute.post("/open/:client_id", doorController.openDoor);
doorRoute.get("/get_access_history/:client_id", doorController.getAccessHistory);
doorRoute.get("/clear_history/:client_id", doorController.clearAccessHistory);