const WebSocket = require("ws");

const PORT = process.env.PORT ? Number(process.env.PORT) : 8080;
const wss = new WebSocket.Server({ port: PORT });
let waitingPlayer = null;
let nextPlayerId = 1;
let nextMatchId = 1;
const TEAM_SIZE = 1;
const NETWORK_MODE = "1v1";
const HERO_POOL = [
  "Naruto", "Sasuke", "Sakura", "Kakashi", "Shikamaru", "Choji", "Ino",
  "Asuma", "Kiba", "Hinata", "Shino", "Lee", "Neji", "Tenten", "Gaara",
  "Temari", "Kankuro", "Sai", "Yamato", "Deidara", "Hidan", "Kisame",
  "Itachi", "Pain", "Konan", "Tobi", "Karin", "Suigetsu", "Jugo",
  "Tobirama", "Hiruzen", "Minato"
];

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

function buildMatchTeams(heroA, heroB, teamSize = TEAM_SIZE) {
  const mainA = heroA || "Naruto";
  const mainB = heroB || "Sasuke";
  const teamA = [mainA];
  const teamB = [mainB];

  const pool = HERO_POOL.filter((h) => h && h !== mainA && h !== mainB);
  let i = 0;
  while (teamA.length < teamSize && i < pool.length) {
    teamA.push(pool[i++]);
  }
  while (teamB.length < teamSize && i < pool.length) {
    teamB.push(pool[i++]);
  }
  while (teamA.length < teamSize) teamA.push("Naruto");
  while (teamB.length < teamSize) teamB.push("Sasuke");

  return { teamA, teamB };
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
  const mapId = 1;
  // Keep queue order intuitive:
  // - player already waiting => Konoha (left)
  // - player who just joined => Akatsuki (right)
  const wsTeam = 1; // Akatsuki
  const otherTeam = 0; // Konoha
  ws.matchId = matchId;
  other.matchId = matchId;
  ws.mapId = mapId;
  other.mapId = mapId;
  ws.team = wsTeam;
  other.team = otherTeam;
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
    mode: NETWORK_MODE,
    matchId,
    opponentId: other.playerId,
    ts: Date.now(),
  });
  safeSend(other, {
    type: "match_found",
    mode: NETWORK_MODE,
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
      if (message.mode && message.mode !== NETWORK_MODE) {
        safeSend(ws, { type: "error", reason: "mode_not_supported", supportedMode: NETWORK_MODE, ts: Date.now() });
        return;
      }
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
        const { teamA, teamB } = buildMatchTeams(
          ws.selectHero,
          ws.opponent.selectHero,
          TEAM_SIZE
        );
        ws.matchStarted = true;
        ws.opponent.matchStarted = true;
        safeSend(ws, { type: "both_locked", ts: Date.now() });
        safeSend(ws.opponent, { type: "both_locked", ts: Date.now() });
        safeSend(ws, {
          type: "match_start",
          mode: NETWORK_MODE,
          teamSize: TEAM_SIZE,
          delayMs: startDelayMs,
          seed,
          mapId: ws.mapId || 1,
          team: ws.team ?? 0,
          yourHero: ws.selectHero || "",
          enemyHero: ws.opponent.selectHero || "",
          yourTeamCsv: teamA.join(","),
          enemyTeamCsv: teamB.join(","),
          ts: Date.now(),
        });
        safeSend(ws.opponent, {
          type: "match_start",
          mode: NETWORK_MODE,
          teamSize: TEAM_SIZE,
          delayMs: startDelayMs,
          seed,
          mapId: ws.opponent.mapId || 1,
          team: ws.opponent.team ?? 1,
          yourHero: ws.opponent.selectHero || "",
          enemyHero: ws.selectHero || "",
          yourTeamCsv: teamB.join(","),
          enemyTeamCsv: teamA.join(","),
          ts: Date.now(),
        });
        console.log(
          `[ws-server] match_start ${ws.matchId} | ` +
            `p${ws.playerId} team=${ws.team} hero=${ws.selectHero} roster=${teamA.join("|")} ` +
            `vs p${ws.opponent.playerId} team=${ws.opponent.team} hero=${ws.opponent.selectHero} roster=${teamB.join("|")}`
        );
      }
      return;
    }

    if (message.type === "input_event") {
      relayToOpponent(ws, {
        type: "input_event",
        action: message.action || "",
        payload: message.payload || {},
        tick: Number.isFinite(message.tick) ? message.tick : 0,
        from: ws.playerId,
        ts: Date.now(),
      });
      return;
    }

    if (message.type === "match_end") {
      const localIsWin = !!message.isWin;
      relayToOpponent(ws, {
        type: "match_end",
        isWin: !localIsWin,
        reason: message.reason || "remote_end",
        from: ws.playerId,
        ts: Date.now(),
      });
      return;
    }

    if (message.type === "match_ui") {
      relayToOpponent(ws, {
        type: "match_ui",
        ui: message.ui || "",
        open: !!message.open,
        from: ws.playerId,
        ts: Date.now(),
      });
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
