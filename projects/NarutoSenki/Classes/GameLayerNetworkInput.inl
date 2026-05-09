#include <ctime>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <chrono>
#include <cmath>

// GameLayerNetworkInput.inl
// - Handles websocket input/event bridge for in-match networking.
// - Player input sync, match_end, and match_ui (pause/gear overlay sync for online).
// - input_event "tick" is a 20 Hz logical slot (50ms) for ordering; joy_update is coalesced to <=20/s.
// - Other world-state packets are ignored to preserve stability.

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
extern "C"
{
void SetNativeWsEventCallback(void (*callback)(const char *eventName, const char *payload));
void MacWsSend(const char *message);
void MacWsDisconnect();
bool MacWsIsConnected();
int GetNetworkForcedMapId();
const char *GetNetworkEnemyHeroName();
}

struct NetInputPacket
{
	uint32_t tick = 0;
	string action;
	string payload;
};

static uint32_t s_remoteMaxTick = 0;
static vector<NetInputPacket> s_pendingRemoteInputs;
static std::chrono::steady_clock::time_point s_lastRemoteInputReceivedAt = std::chrono::steady_clock::now();
static bool s_lastSentJoyRelease = true;
static float s_lastSentJoyX = 0.0f;
static float s_lastSentJoyY = 0.0f;
static bool s_isApplyingRemoteMatchEnd = false;

static constexpr int kWsInputTicksPerSecond = 20;
static constexpr int kWsInputTickIntervalMs = 1000 / kWsInputTicksPerSecond;

static uint32_t s_wsNetLogicalTick = 0;
static std::chrono::steady_clock::time_point s_wsNetTickEpoch{};
static bool s_wsNetTickEpochInit = false;

static std::chrono::steady_clock::time_point s_lastJoyWireSend{};
static float s_pendingJoyWireX = 0.f;
static float s_pendingJoyWireY = 0.f;
static bool s_pendingJoyWire = false;
static bool s_didSendJoyWire = false;

static void advanceWsNetworkTick20Hz()
{
	const auto now = std::chrono::steady_clock::now();
	if (!s_wsNetTickEpochInit)
	{
		s_wsNetTickEpoch = now;
		s_wsNetTickEpochInit = true;
		s_wsNetLogicalTick = 1u;
		return;
	}
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_wsNetTickEpoch).count();
	const uint32_t slot = 1u + (uint32_t)std::max<int64_t>(0, (int64_t)ms / (int64_t)kWsInputTickIntervalMs);
	if (slot > s_wsNetLogicalTick)
		s_wsNetLogicalTick = slot;
}

static void marshalAndSendInputEvent(const string &action, const string &payload)
{
	auto message =
		string("{\"type\":\"input_event\",\"action\":\"") + action + "\",\"payload\":" + payload +
		",\"tick\":" + to_string(s_wsNetLogicalTick) + ",\"ts\":" + to_string((int)time(nullptr)) + "}";
	MacWsSend(message.c_str());
}

static void flushPendingJoyWire(bool force)
{
	if (!s_pendingJoyWire)
		return;
	if (!MacWsIsConnected())
		return;
	if (_gLayer && _gLayer->shouldBlockNetworkBattleInputEcho())
		return;
	const auto now = std::chrono::steady_clock::now();
	const long dt = (long)std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastJoyWireSend).count();
	if (!force && s_didSendJoyWire && dt < kWsInputTickIntervalMs)
		return;

	advanceWsNetworkTick20Hz();
	marshalAndSendInputEvent("joy_update",
							 format("{{\"x\":{:.3f},\"y\":{:.3f}}}", s_pendingJoyWireX, s_pendingJoyWireY));
	s_lastSentJoyRelease = false;
	s_lastSentJoyX = s_pendingJoyWireX;
	s_lastSentJoyY = s_pendingJoyWireY;
	s_lastJoyWireSend = now;
	s_didSendJoyWire = true;
	s_pendingJoyWire = false;
}

static const char *jsonStringField(const char *payload, const char *field, string &out)
{
	if (!payload || !field)
		return nullptr;
	auto key = format("\"{}\":\"", field);
	const char *start = strstr(payload, key.c_str());
	if (!start)
		return nullptr;
	start += key.size();
	const char *end = strchr(start, '"');
	if (!end)
		return nullptr;
	out.assign(start, end - start);
	return end;
}

static bool jsonIntField(const char *payload, const char *field, int &out)
{
	if (!payload || !field)
		return false;
	auto key = format("\"{}\":", field);
	const char *start = strstr(payload, key.c_str());
	if (!start)
		return false;
	start += key.size();
	out = atoi(start);
	return true;
}

static bool jsonFloatField(const char *payload, const char *field, float &out)
{
	if (!payload || !field)
		return false;
	auto key = format("\"{}\":", field);
	const char *start = strstr(payload, key.c_str());
	if (!start)
		return false;
	start += key.size();
	out = strtof(start, nullptr);
	return true;
}

static bool jsonBoolField(const char *payload, const char *field, bool &out)
{
	if (!payload || !field)
		return false;
	auto key = format("\"{}\":", field);
	const char *start = strstr(payload, key.c_str());
	if (!start)
		return false;
	start += key.size();
	if (strncmp(start, "true", 4) == 0)
	{
		out = true;
		return true;
	}
	if (strncmp(start, "false", 5) == 0)
	{
		out = false;
		return true;
	}
	return false;
}

static CharacterBase *getRemoteControlTarget(GameLayer *layer)
{
	if (!layer)
		return nullptr;
	const char *remoteHeroName = GetNetworkEnemyHeroName();
	CharacterBase *fallback = nullptr;
	for (auto hero : layer->_CharacterArray)
	{
		if (!hero)
			continue;
		if (hero == layer->currentPlayer)
			continue;
		if (hero->getGroup() == layer->playerGroup)
			continue;
		if (hero->getState() == State::DEAD)
			continue;
		if (!fallback)
			fallback = hero;
		if (remoteHeroName && strlen(remoteHeroName) > 0 && hero->getName() == remoteHeroName)
		{
			return hero;
		}
	}
	return fallback;
}

static void applyRemoteInputAction(GameLayer *layer, const NetInputPacket &packet)
{
	if (!layer || !layer->_isStarted)
		return;

	auto target = getRemoteControlTarget(layer);
	if (!target)
		return;
	target->unschedule(schedule_selector(CharacterBase::setAI));
	target->_isAI = false;

	if (packet.action == "joy_update")
	{
		float x = 0.0f;
		float y = 0.0f;
		if (jsonFloatField(packet.payload.c_str(), "x", x) && jsonFloatField(packet.payload.c_str(), "y", y))
		{
			target->walk(Vec2(x, y));
		}
	}
	else if (packet.action == "joy_release")
	{
		if (target->getState() == State::WALK)
			target->idle();
	}
	else if (packet.action == "attack_click")
	{
		int type = 0;
		if (jsonIntField(packet.payload.c_str(), "type", type))
			target->attack((ABType)type);
	}
	else if (packet.action == "gear_click")
	{
		int type = 0;
		if (jsonIntField(packet.payload.c_str(), "type", type))
			target->useGear((GearType)type);
	}
}

static void queueRemoteInputEvent(GameLayer *layer, const char *payload)
{
	if (!layer || !payload || !layer->_isStarted)
		return;
	if (strstr(payload, "\"type\":\"input_event\"") == nullptr)
		return;

	NetInputPacket packet;
	if (!jsonStringField(payload, "action", packet.action))
		return;
	int tick = 0;
	if (!jsonIntField(payload, "tick", tick) || tick <= 0)
	{
		packet.tick = s_remoteMaxTick + 1;
	}
	else
	{
		packet.tick = (uint32_t)tick;
	}

	const char *payloadKey = "\"payload\":";
	const char *payloadStart = strstr(payload, payloadKey);
	if (!payloadStart)
		return;
	payloadStart += strlen(payloadKey);
	const char *tsKey = strstr(payloadStart, ",\"ts\":");
	if (!tsKey)
		return;
	packet.payload.assign(payloadStart, tsKey - payloadStart);

	s_remoteMaxTick = MAX(s_remoteMaxTick, packet.tick);
	s_pendingRemoteInputs.push_back(packet);
	s_lastRemoteInputReceivedAt = std::chrono::steady_clock::now();
}

static void processRemoteInputQueue(GameLayer *layer)
{
	if (!layer)
		return;

	flushPendingJoyWire(false);

	// Remote human opponent is represented as COM locally.
	// Keep it out of AI mode continuously, including right after respawn.
	if (auto remotePlayer = getRemoteControlTarget(layer))
	{
		if (remotePlayer->_isAI)
		{
			remotePlayer->unschedule(schedule_selector(CharacterBase::setAI));
			remotePlayer->_isAI = false;
		}
	}

	if (s_pendingRemoteInputs.empty())
		return;

	bool hasPendingRelease = false;
	for (const auto &packet : s_pendingRemoteInputs)
	{
		if (packet.action == "joy_release")
		{
			hasPendingRelease = true;
			break;
		}
	}

	const auto idleMs = std::chrono::duration_cast<std::chrono::milliseconds>(
							std::chrono::steady_clock::now() - s_lastRemoteInputReceivedAt)
							.count();
	uint32_t applyUntilTick = 0;
	if (s_remoteMaxTick >= 1)
	{
		applyUntilTick = s_remoteMaxTick - 1;
	}
	if (hasPendingRelease || idleMs >= 50)
	{
		applyUntilTick = s_remoteMaxTick;
	}
	if (applyUntilTick == 0)
		return;
	std::stable_sort(s_pendingRemoteInputs.begin(), s_pendingRemoteInputs.end(),
					 [](const NetInputPacket &a, const NetInputPacket &b) { return a.tick < b.tick; });

	vector<NetInputPacket> remaining;
	remaining.reserve(s_pendingRemoteInputs.size());
	for (const auto &packet : s_pendingRemoteInputs)
	{
		if (packet.tick <= applyUntilTick)
		{
			applyRemoteInputAction(layer, packet);
		}
		else
		{
			remaining.push_back(packet);
		}
	}
	s_pendingRemoteInputs.swap(remaining);
}

static void sendNetworkInputEvent(const string &action, const string &payload = "{}")
{
	if (!MacWsIsConnected())
		return;
	if (_gLayer && _gLayer->shouldBlockNetworkBattleInputEcho())
	{
		if (action != "joy_release")
			return;
	}
	advanceWsNetworkTick20Hz();
	marshalAndSendInputEvent(action, payload);
}

static void sendNetworkJoyUpdateEvent(float x, float y)
{
	if (!MacWsIsConnected())
		return;
	if (_gLayer && _gLayer->shouldBlockNetworkBattleInputEcho())
		return;
	// Coalesce to 20 Hz on the wire; keep refreshing pending so a held direction still flushes every 50ms.
	s_pendingJoyWire = true;
	s_pendingJoyWireX = x;
	s_pendingJoyWireY = y;
	flushPendingJoyWire(false);
}

static void sendNetworkJoyReleaseEvent()
{
	if (s_lastSentJoyRelease)
		return;
	if (MacWsIsConnected() && (!_gLayer || !_gLayer->shouldBlockNetworkBattleInputEcho()))
		flushPendingJoyWire(true);
	s_lastSentJoyRelease = true;
	sendNetworkInputEvent("joy_release");
}

static void sendNetworkMatchEnd(bool isWin, const char *reason = "game_over")
{
	if (!MacWsIsConnected())
		return;
	auto message = string("{\"type\":\"match_end\",\"isWin\":") +
				   (isWin ? "true" : "false") +
				   ",\"reason\":\"" + (reason ? string(reason) : string("game_over")) + "\"" +
				   ",\"ts\":" + to_string((int)time(nullptr)) + "}";
	MacWsSend(message.c_str());
}

static bool s_suppressNetworkMatchUI = false;

static void sendNetworkMatchUIIfActive(const char *which, bool open)
{
	if (s_suppressNetworkMatchUI)
		return;
	if (!MacWsIsConnected() || !_gLayer || !_gLayer->_isStarted)
		return;
	string safeUi = which ? which : "";
	auto message = string("{\"type\":\"match_ui\",\"ui\":\"") + safeUi + "\",\"open\":" +
				   (open ? "true" : "false") +
				   ",\"ts\":" + to_string((int)time(nullptr)) + "}";
	MacWsSend(message.c_str());
}

static void applyRemoteMatchUI(GameLayer *layer, const char *which, bool open)
{
	if (!which || !layer || !layer->_isStarted || layer->_isExiting)
		return;
	// Network match now keeps gameplay running: ignore pause/gear overlay sync.
	if (MacWsIsConnected())
		return;
	s_suppressNetworkMatchUI = true;
	if (strcmp(which, "pause") == 0)
	{
		if (open)
		{
			if (!layer->_isPause)
				layer->onPause();
		}
		else
		{
			if (layer->_isPause)
				layer->resumeFromPause();
		}
	}
	else if (strcmp(which, "gear") == 0)
	{
		if (open)
		{
			if (!layer->_isGear && layer->_enableGear)
				layer->onGear();
		}
		else
		{
			if (layer->_isGear)
				layer->dismissGearOverlay();
		}
	}
	s_suppressNetworkMatchUI = false;
}

static void onNativeWsEvent(const char *eventName, const char *payload)
{
	if (!eventName)
		return;

	// Lobby/select still handle disconnect in Lua; during battle Lua has no WS delegate.
	if (strcmp(eventName, "close") == 0 || strcmp(eventName, "error") == 0)
	{
		if (_gLayer && _gLayer->_isStarted && !_gLayer->_isExiting)
		{
			if (!_gLayer->_hasGameOverTriggered)
				_gLayer->onGameOver(true);
			MacWsDisconnect();
		}
		return;
	}

	if (!payload)
		return;
	if (strcmp(eventName, "message") != 0)
		return;
	if (strstr(payload, "\"type\":\"opponent_left\"") != nullptr)
	{
		if (_gLayer && _gLayer->_isStarted && !_gLayer->_isExiting)
		{
			if (!_gLayer->_hasGameOverTriggered)
				_gLayer->onGameOver(true);
			MacWsDisconnect();
		}
		return;
	}
	if (strstr(payload, "\"type\":\"match_end\"") != nullptr)
	{
		if (_gLayer && _gLayer->_isStarted && !_gLayer->_isExiting)
		{
			bool isWin = false;
			jsonBoolField(payload, "isWin", isWin);
			s_isApplyingRemoteMatchEnd = true;
			_gLayer->onGameOver(isWin);
			s_isApplyingRemoteMatchEnd = false;
		}
		return;
	}
	if (strstr(payload, "\"type\":\"match_ui\"") != nullptr)
	{
		if (_gLayer && _gLayer->_isStarted && !_gLayer->_isExiting)
		{
			string ui;
			bool open = false;
			if (jsonStringField(payload, "ui", ui) && jsonBoolField(payload, "open", open))
			{
				applyRemoteMatchUI(_gLayer, ui.c_str(), open);
			}
		}
		return;
	}
	if (strstr(payload, "\"type\":\"hero_snap\"") != nullptr)
	{
		if (!_gLayer || !_gLayer->_isStarted || _gLayer->_isExiting || !_gLayer->currentMap)
			return;
		float nx = 0.f;
		float ny = 0.f;
		if (!jsonFloatField(payload, "x", nx) || !jsonFloatField(payload, "y", ny))
			return;

		auto *remote = getRemoteControlTarget(_gLayer);
		if (!remote || remote->getState() != State::WALK)
			return;

		auto *map = _gLayer->currentMap;
		const float mw = float(map->getMapSize().width * map->getTileSize().width);
		float posX = MIN(mw, MAX(0.f, nx));
		float poxY = MIN(float(map->getTileSize().height * 5.5f), MAX(0.f, ny));
		const Vec2 next(posX, poxY);
		const Vec2 cur = remote->getPosition();
		const float rdx = cur.x - next.x;
		const float rdy = cur.y - next.y;
		if (rdx * rdx + rdy * rdy < 225.f)
			return;
		remote->setPosition(next);
		_gLayer->reorderChild(remote, -(int)poxY);
		return;
	}
	if (strstr(payload, "\"type\":\"input_event\"") == nullptr)
	{
		return;
	}
	queueRemoteInputEvent(_gLayer, payload);
}

static void resetNetworkInputStateOnEnter()
{
	SetNativeWsEventCallback(&onNativeWsEvent);
	s_wsNetLogicalTick = 0;
	s_wsNetTickEpochInit = false;
	s_lastJoyWireSend = {};
	s_pendingJoyWire = false;
	s_didSendJoyWire = false;
	s_remoteMaxTick = 0;
	s_pendingRemoteInputs.clear();
	s_lastRemoteInputReceivedAt = std::chrono::steady_clock::now();
	s_lastSentJoyRelease = true;
	s_lastSentJoyX = 0.0f;
	s_lastSentJoyY = 0.0f;
}

static void resetNetworkInputStateOnExit()
{
	SetNativeWsEventCallback(nullptr);
	s_pendingRemoteInputs.clear();
}
#else
static void sendNetworkInputEvent(const string &, const string & = "{}") {}
static void sendNetworkJoyUpdateEvent(float, float) {}
static void sendNetworkJoyReleaseEvent() {}
static void sendNetworkMatchEnd(bool, const char * = "game_over") {}
static void sendNetworkMatchUIIfActive(const char *, bool) {}
static void resetNetworkInputStateOnEnter() {}
static void resetNetworkInputStateOnExit() {}
#endif
