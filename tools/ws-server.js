const WebSocket = require("ws");

const PORT = process.env.PORT ? Number(process.env.PORT) : 8080;
const wss = new WebSocket.Server({ port: PORT });

// ─── Constants ────────────────────────────────────────────────────────────────
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

// ─── FIX #1: Queue pakai array, bukan single variable (race condition fix) ────
let matchmakingQueue = [];

// ─── FIX #2: Heartbeat & Rate Limit config ────────────────────────────────────
const HEARTBEAT_INTERVAL_MS = 15_000;   // cek koneksi tiap 15 detik
const QUEUE_TIMEOUT_MS      = 30_000;   // kick dari queue setelah 30 detik
const RATE_LIMIT_MAX        = 120;      // max pesan per detik per player
const RATE_LIMIT_WINDOW_MS  = 1_000;

console.log(`[ws-server] listening on ws://127.0.0.1:${PORT}`);

// ─── Helpers ──────────────────────────────────────────────────────────────────
function safeSend(ws, obj) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(obj));
  }
}

// FIX #1: leaveQueue sekarang filter dari array + clear timeout
function leaveQueue(ws) {
  clearTimeout(ws.queueTimeout);
  ws.queueTimeout = null;
  matchmakingQueue = matchmakingQueue.filter((p) => p !== ws);
}

// FIX #4: cleanupMatch dipanggil di match_end & breakMatch agar tidak memory leak
function cleanupMatch(ws) {
  const other = ws.opponent;
  ws.opponent  = null;
  ws.matchId   = null;
  ws.matchStarted = false;
  ws.selectLocked = false;
  ws.selectHero   = "";

  if (other && other.opponent === ws) {
    other.opponent  = null;
    other.matchId   = null;
    other.matchStarted = false;
    other.selectLocked = false;
    other.selectHero   = "";
  }
}

function breakMatch(ws, reason = "opponent_left") {
  if (!ws || !ws.opponent) return;

  const other = ws.opponent;
  safeSend(other, { type: reason, ts: Date.now() });
  cleanupMatch(ws);
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
  while (teamA.length < teamSize && i < pool.length) teamA.push(pool[i++]);
  while (teamB.length < teamSize && i < pool.length) teamB.push(pool[i++]);
  while (teamA.length < teamSize) teamA.push("Naruto");
  while (teamB.length < teamSize) teamB.push("Sasuke");

  return { teamA, teamB };
}

// FIX #1: tryMatchmake pakai array queue
function tryMatchmake(ws) {
  // Cegah masuk queue dua kali
  if (matchmakingQueue.includes(ws)) {
    safeSend(ws, { type: "queue_waiting", ts: Date.now() });
    return;
  }

  // FIX #2: Queue timeout — kick player jika terlalu lama menunggu
  ws.queueTimeout = setTimeout(() => {
    leaveQueue(ws);
    safeSend(ws, { type: "queue_timeout", ts: Date.now() });
    console.log(`[ws-server] p${ws.playerId} timed out from queue`);
  }, QUEUE_TIMEOUT_MS);

  matchmakingQueue.push(ws);

  if (matchmakingQueue.length >= 2) {
    // Ambil dua player paling depan secara atomik
    const [other, current] = matchmakingQueue.splice(0, 2);
    clearTimeout(other.queueTimeout);
    clearTimeout(current.queueTimeout);
    other.queueTimeout   = null;
    current.queueTimeout = null;
    startMatch(other, current);
  } else {
    safeSend(ws, { type: "queue_waiting", ts: Date.now() });
  }
}

function startMatch(other, ws) {
  const matchId = `M${nextMatchId++}`;
  const mapId   = 1;

  // other = player yang sudah nunggu → Konoha (team 0)
  // ws    = player yang baru join   → Akatsuki (team 1)
  other.team = 0;
  ws.team    = 1;

  ws.matchId    = matchId; ws.mapId = mapId;
  other.matchId = matchId; other.mapId = mapId;
  ws.opponent   = other;   other.opponent = ws;
  ws.selectLocked   = false; other.selectLocked   = false;
  ws.selectHero     = "";    other.selectHero     = "";
  ws.matchStarted   = false; other.matchStarted   = false;

  safeSend(ws, {
    type: "match_found", mode: NETWORK_MODE,
    matchId, opponentId: other.playerId, ts: Date.now(),
  });
  safeSend(other, {
    type: "match_found", mode: NETWORK_MODE,
    matchId, opponentId: ws.playerId, ts: Date.now(),
  });

  console.log(
    `[ws-server] match found: ${matchId} | p${other.playerId}(Konoha) vs p${ws.playerId}(Akatsuki)`
  );
}

// ─── Server ───────────────────────────────────────────────────────────────────
wss.on("connection", (ws) => {
  ws.playerId    = nextPlayerId++;
  ws.matchId     = null;
  ws.opponent    = null;
  ws.selectLocked = false;
  ws.selectHero  = "";
  ws.matchStarted = false;
  ws.queueTimeout = null;

  // FIX #2: Rate limiter state
  ws.msgCount   = 0;
  ws.msgResetAt = Date.now() + RATE_LIMIT_WINDOW_MS;

  // FIX #3: Heartbeat — deteksi zombie connection
  ws.isAlive = true;
  ws.on("pong", () => { ws.isAlive = true; });

  const heartbeatTimer = setInterval(() => {
    if (!ws.isAlive) {
      console.log(`[ws-server] zombie connection p${ws.playerId}, terminating`);
      breakMatch(ws, "opponent_left");
      leaveQueue(ws);
      ws.terminate();
      return;
    }
    ws.isAlive = false;
    ws.ping();
  }, HEARTBEAT_INTERVAL_MS);

  console.log(`[ws-server] client connected p${ws.playerId}`);
  safeSend(ws, { type: "welcome", playerId: ws.playerId, ts: Date.now() });

  ws.on("message", (data) => {
    // FIX #2: Rate limiting
    const now = Date.now();
    if (now > ws.msgResetAt) {
      ws.msgCount   = 0;
      ws.msgResetAt = now + RATE_LIMIT_WINDOW_MS;
    }
    ws.msgCount++;
    if (ws.msgCount > RATE_LIMIT_MAX) {
      // Silent drop — jangan log tiap pesan agar tidak banjiri console
      return;
    }

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

    /** RTT sample for in-game HUD — reply to sender only (not relayed to opponent). */
    if (message.type === "latency_ping") {
      safeSend(ws, { type: "latency_pong", ts: Date.now() });
      return;
    }

    if (message.type === "queue_join") {
      if (message.mode && message.mode !== NETWORK_MODE) {
        safeSend(ws, {
          type: "error", reason: "mode_not_supported",
          supportedMode: NETWORK_MODE, ts: Date.now(),
        });
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
      ws.selectHero   = message.hero || "";
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
      ws.selectHero   = message.hero || "";
      ws.selectLocked = true;
      relayToOpponent(ws, {
        type: "select_lock",
        hero: message.hero || "",
        from: ws.playerId,
        ts: Date.now(),
      });

      if (
        ws.opponent &&
        ws.opponent.selectLocked &&
        !ws.matchStarted &&
        !ws.opponent.matchStarted
      ) {
        const startDelayMs = 1500;
        const seed = Math.floor(Math.random() * 2147483647);
        const { teamA, teamB } = buildMatchTeams(
          ws.selectHero,
          ws.opponent.selectHero,
          TEAM_SIZE
        );

        ws.matchStarted          = true;
        ws.opponent.matchStarted = true;

        safeSend(ws,          { type: "both_locked", ts: Date.now() });
        safeSend(ws.opponent, { type: "both_locked", ts: Date.now() });

        safeSend(ws, {
          type: "match_start", mode: NETWORK_MODE, teamSize: TEAM_SIZE,
          delayMs: startDelayMs, seed, mapId: ws.mapId || 1,
          team: ws.team ?? 0,
          yourHero: ws.selectHero || "",
          enemyHero: ws.opponent.selectHero || "",
          yourTeamCsv: teamA.join(","),
          enemyTeamCsv: teamB.join(","),
          ts: Date.now(),
        });
        safeSend(ws.opponent, {
          type: "match_start", mode: NETWORK_MODE, teamSize: TEAM_SIZE,
          delayMs: startDelayMs, seed, mapId: ws.opponent.mapId || 1,
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

    if (message.type === "hero_snap") {
      const x = Number(message.x);
      const y = Number(message.y);
      if (!Number.isFinite(x) || !Number.isFinite(y)) return;
      relayToOpponent(ws, { type: "hero_snap", x, y, from: ws.playerId, ts: Date.now() });
      return;
    }

    if (message.type === "knock_snap") {
      const x = Number(message.x);
      const y = Number(message.y);
      if (!Number.isFinite(x) || !Number.isFinite(y)) return;
      relayToOpponent(ws, { type: "knock_snap", x, y, from: ws.playerId, ts: Date.now() });
      return;
    }

    if (message.type === "battle_stat") {
      relayToOpponent(ws, {
        type: "battle_stat",
        hp: Number(message.hp) || 0,
        maxHp: Number(message.maxHp) || 0,
        kills: Number(message.kills) || 0,
        deaths: Number(message.deaths) || 0,
        from: ws.playerId,
        ts: Date.now(),
      });
      return;
    }

    /** Lane frog waves — Konoha client is authoritative; relay so Akatsuki mirrors the same spawn cadence. */
    if (message.type === "flog_wave") {
      relayToOpponent(ws, {
        type: "flog_wave",
        seq: Number(message.seq) || 0,
        from: ws.playerId,
        ts: Date.now(),
      });
      return;
    }

    /** Konoha ~10Hz frog pose/HP CSV — follower applies so lane matches authority (no ghost hits). */
    if (message.type === "flog_snap") {
      const d = typeof message.d === "string" ? message.d : "";
      relayToOpponent(ws, {
        type: "flog_snap",
        d,
        from: ws.playerId,
        ts: Date.now(),
      });
      return;
    }

    /** Hardcore guardian Roshi/Han — must relay; unmatched types fall through to echo (sender only). */
    if (message.type === "guardian_spawn") {
      const n = Math.floor(Number(message.idx));
      if (!Number.isFinite(n)) return;
      relayToOpponent(ws, {
        type: "guardian_spawn",
        idx: ((n % 2) + 2) % 2,
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
      // FIX #4: Bersihkan state match setelah selesai
      cleanupMatch(ws);
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
    // FIX #3: Hentikan heartbeat timer saat koneksi tutup
    clearInterval(heartbeatTimer);
    breakMatch(ws, "opponent_left");
    leaveQueue(ws);
    console.log(`[ws-server] client disconnected p${ws.playerId}`);
  });
});
