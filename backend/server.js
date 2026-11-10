const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const sqlite3 = require('sqlite3').verbose();
const cors = require('cors');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

const app = express();
app.use(cors());
app.use(express.json());
app.use(express.static('../frontend'));

const server = http.createServer(app);
const io = new Server(server, { cors: { origin: '*' } });

// 1. Inicialização da Base de Dados SQLite
const db = new sqlite3.Database('./escudo_logs.db', (err) => {
  if (err) console.error('Erro ao conectar com SQLite:', err.message);
  else console.log('Base de Dados SQLite pronta para auditoria.');
});

db.run(`CREATE TABLE IF NOT EXISTS logs (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
  hora_esp TEXT,
  estado TEXT,
  temperatura REAL,
  humidade REAL,
  origem TEXT DEFAULT 'hardware'
)`);

// 2. Conexão com a Porta Serial USB do ESP32
const PORTA_SERIAL = 'COM3'; // Substituir pela porta COM do vosso PC

try {
  const port = new SerialPort({ path: PORTA_SERIAL, baudRate: 115200 });
  const parser = port.pipe(new ReadlineParser({ delimiter: '\n' }));

  parser.on('data', (line) => {
    try {
      const dados = JSON.parse(line.trim());
      
      // Registo na Base de Dados
      db.run(
        `INSERT INTO logs (hora_esp, estado, temperatura, humidade, origem) VALUES (?, ?, ?, ?, 'hardware')`,
        [dados.hora, dados.estado, dados.temp, dados.hum]
      );

      // Envio instantâneo para a Dashboard Web
      io.emit('telemetria', dados);
    } catch (e) {
      // Ignora ruído de inicialização Serial
    }
  });
} catch (error) {
  console.log('Modo Simulador Ativo (ESP32 não detetado em ' + PORTA_SERIAL + ')');
}

// 3. Rotas da API REST

// Consulta o histórico de logs da base de dados
app.get('/api/logs', (req, res) => {
  db.all('SELECT * FROM logs ORDER BY id DESC LIMIT 50', [], (err, rows) => {
    if (err) return res.status(500).json({ error: err.message });
    res.json(rows);
  });
});

// Injeta um Ciberataque "Fake" para testes de auditoria
app.post('/api/simular-ataque', (req, res) => {
  const { tipoAtaque, horaPersonalizada } = req.body;
  const hora = horaPersonalizada || new Date().toLocaleTimeString();

  db.run(
    `INSERT INTO logs (hora_esp, estado, temperatura, humidade, origem) VALUES (?, ?, 0, 0, 'simulador_fake')`,
    [hora, tipoAtaque]
  );

  const eventoFake = { hora, estado: tipoAtaque, temp: 0, hum: 0, origem: 'simulador_fake' };
  io.emit('telemetria', eventoFake);

  res.json({ status: 'Ataque simulado com sucesso', evento: eventoFake });
});

server.listen(3000, () => {
  console.log('Host Central a correr em: http://localhost:3000');
});