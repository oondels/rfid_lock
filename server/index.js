const express = require("express");
const cors = require("cors");
const pool = require("./conn/db.js");
const port = 2399;
const app = express();

app.use(cors(
{ origin: "*" }
));
app.use(express.json());

app.linsten((port) => {
    try {
    const rfid = req.body.rfid;

    const queryRfidValue = await pool.query(
      `
      SELECT nome, rfid
      FROM database
      WHERE rfid = $1
    `,
      [rfid]
    );

    if (queryRfidValue.rows.length === 0) {
      return res.status(403).json({ message: "RFID não encontrado." });
    }
    const name =
      queryRfidValue.rows[0].nome.split(" ")[0] +
      " " +
      queryRfidValue.rows[0].nome.split(" ")[queryRfidValue.rows[0].nome.split(" ").length - 1];

    return res.status(200).json({ message: "RFID encontrado.", nome: name });
  } catch (error) {
    console.error("Erro interno no servidor.");
    return res.status(500).json({ message: "Erro interno no servidor.", error: error });
  }
})
