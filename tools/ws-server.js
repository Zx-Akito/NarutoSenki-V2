const WebSocket = require("ws");

const PORT = process.env.PORT ? Number(process.env.PORT) : 8080;
const wss = new WebSocket.Server({ port: PORT });
let waitingPlayer = null;
let nextPlayerId = 1;
let nextMatchId = 1;

console.log(`[ws-server] listening on ws://127.0.0.1:${PORT}`);

function safeSend(ws, obj) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(obj));
  }
}

function leaveQueue(ws) {
  if (waitingPlayer === ws) {
    waitingPlayer = null;
  }
}

function breakMatch(ws, reason = "opponent_left") {
  if (!ws || !ws.opponent) return;

  const other = ws.opponent;
  ws.opponent = null;
  ws.matchId = null;

  if (other.opponent === ws) {
    other.opponent = null;
    other.matchId = null;
  }

  safeSend(other, { type: reason, ts: Date.now() });
  // Force disconnect opponent when one player leaves match lobby.
  if (other.readyState === WebSocket.OPEN) {
    other.close(1000, "opponent_left");
  }
}

function relayToOpponent(ws, payload) {
  if (!ws || !ws.opponent || ws.opponent.readyState !== WebSocket.OPEN) return;
  safeSend(ws.opponent, payload);
}

function tryMatchmake(ws) {
  if (!waitingPlayer || waitingPlayer === ws) {
    waitingPlayer = ws;
    safeSend(ws, { type: "queue_waiting", ts: Date.now() });
    return;
  }

  const other = waitingPlayer;
  waitingPlayer = null;

  const matchId = `M${nextMatchId++}`;
  ws.matchId = matchId;
  other.matchId = matchId;
  ws.opponent = other;
  other.opponent = ws;
  ws.selectLocked = false;
  other.selectLocked = false;
  ws.selectHero = "";
  other.selectHero = "";
  ws.matchStarted = false;
  other.matchStarted = false;

  safeSend(ws, {
    type: "match_found",
    matchId,
    opponentId: other.playerId,
    ts: Date.now(),
  });
  safeSend(other, {
    type: "match_found",
    matchId,
    opponentId: ws.playerId,
    ts: Date.now(),
  });

  console.log(
    `[ws-server] match found: ${matchId} | p${other.playerId} vs p${ws.playerId}`
  );
}

wss.on("connection", (ws) => {
  ws.playerId = nextPlayerId++;
  ws.matchId = null;
  ws.opponent = null;
  ws.selectLocked = false;
  ws.selectHero = "";
  ws.matchStarted = false;

  console.log(`[ws-server] client connected p${ws.playerId}`);
  safeSend(ws, { type: "welcome", playerId: ws.playerId, ts: Date.now() });

  ws.on("message", (data) => {
    const text = data.toString();
    console.log(`[ws-server] recv p${ws.playerId}:`, text);

    let message;
    try {
      message = JSON.parse(text);
    } catch {
      safeSend(ws, { type: "error", reason: "invalid_json", ts: Date.now() });
      return;
    }

    if (message.type === "ping") {
      safeSend(ws, { type: "pong", ts: Date.now() });
      return;
    }

    if (message.type === "queue_join") {
      tryMatchmake(ws);
      return;
    }

    if (message.type === "queue_leave") {
      breakMatch(ws, "opponent_left");
      leaveQueue(ws);
      safeSend(ws, { type: "queue_left", ts: Date.now() });
      return;
    }

    if (message.type === "select_update") {
      ws.selectHero = message.hero || "";
      ws.selectLocked = false;
      ws.matchStarted = false;
      relayToOpponent(ws, {
        type: "select_update",
        hero: message.hero || "",
        from: ws.playerId,
        ts: Date.now(),
      });
      return;
    }

    if (message.type === "select_lock") {
      ws.selectHero = message.hero || "";
      ws.selectLocked = true;
      relayToOpponent(ws, {
        type: "select_lock",
        hero: message.hero || "",
        from: ws.playerId,
        ts: Date.now(),
      });
      if (ws.opponent && ws.opponent.selectLocked && !ws.matchStarted && !ws.opponent.matchStarted) {
        const startDelayMs = 1500;
        const seed = Math.floor(Math.random() * 2147483647);
        ws.matchStarted = true;
        ws.opponent.matchStarted = true;
        safeSend(ws, { type: "both_locked", ts: Date.now() });
        safeSend(ws.opponent, { type: "both_locked", ts: Date.now() });
        safeSend(ws, {
          type: "match_start",
          delayMs: startDelayMs,
          seed,
          ts: Date.now(),
        });
        safeSend(ws.opponent, {
          type: "match_start",
          delayMs: startDelayMs,
          seed,
          ts: Date.now(),
        });
      }
      return;
    }

    safeSend(ws, { type: "echo", payload: message, ts: Date.now() });
  });

  ws.on("close", () => {
    breakMatch(ws, "opponent_left");
    leaveQueue(ws);
    console.log(`[ws-server] client disconnected p${ws.playerId}`);
  });
});
