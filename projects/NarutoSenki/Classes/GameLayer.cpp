#include "Defines.h"
#include <chrono>
#include <cstdio>
#include <ctime>
#include <sstream>
#include "CharacterBase.h"
#include "Core/Warrior/Flog.hpp"
#include "Data/Config.h"
#include "GameLayer.h"
#include "BGLayer.h"
#include "HudLayer.h"
#include "StartMenu.h"
#include "Core/Provider.hpp"
#include "GameMode/GameModeImpl.h"
#include "Constants/UiFlowKeys.hpp"
#include "Systems/BattleRuntimeSystem.hpp"
#include "Systems/SpawnSystem.hpp"
#include "Systems/SessionState.hpp"
#include "Enums/TowerEnum.h"

// GameLayer.cpp
// - Owns core battle lifecycle and gameplay flow.
// - Delegates network input details to GameLayerNetworkInput.inl.
// - Delegates local input/key handling to GameLayerInputControl.inl.

GameLayer *_gLayer = nullptr;
bool _isFullScreen = false;

namespace
{
static Flog *findFlogByNetworkSlot(GameLayer *layer, uint32_t wave, unsigned slot)
{
	if (!layer || slot >= (unsigned)(2 * kFlogCount))
		return nullptr;
	for (auto *p : layer->_KonohaFlogArray)
	{
		if (p && p->_netWaveSeq == wave && (unsigned)p->_netSlot == slot)
			return p;
	}
	for (auto *p : layer->_AkatsukiFlogArray)
	{
		if (p && p->_netWaveSeq == wave && (unsigned)p->_netSlot == slot)
			return p;
	}
	return nullptr;
}
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
extern "C" bool MacWsIsConnected();
extern "C" void MacWsSend(const char *message);
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
extern "C" bool NativeBridgeWsIsConnected(void);
extern "C" void NativeBridgeWsSend(const char *message);
#endif

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
extern "C" int GetNetworkForcedTeam();

static inline bool GamePlatformWsConnected()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	return MacWsIsConnected();
#else
	return NativeBridgeWsIsConnected();
#endif
}

static inline void GamePlatformWsSend(const char *msg)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	MacWsSend(msg);
#else
	NativeBridgeWsSend(msg);
#endif
}
#endif

#include "GameLayerNetworkInput.inl"

extern "C" bool GetNetworkOpponentIsBot();

bool GameLayer::shouldBlockNetworkBattleInputEcho() const
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	return _isStarted && GamePlatformWsConnected() && (_isPause || _isGear);
#else
	return false;
#endif
}

void GameLayer::sendNetworkOwnedHeroPositionSnapIfNeeded(const Vec2 &worldPos, HeroSnapKind kind)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (!GamePlatformWsConnected() || !_isStarted || _isExiting)
		return;
	using namespace std::chrono;
	static steady_clock::time_point s_lastSentAt = steady_clock::now();
	static Vec2 s_lastSentPos(0, 0);
	static bool s_haveLastSentPos = false;
	const auto now = steady_clock::now();
	const auto msSince = duration_cast<milliseconds>(now - s_lastSentAt).count();
	const float ddx = worldPos.x - s_lastSentPos.x;
	const float ddy = worldPos.y - s_lastSentPos.y;
	const float drift2 = ddx * ddx + ddy * ddy;

	int minIntervalMs = 250;
	float minQuietDrift2 = 100.f;
	if (kind == HeroSnapKind::TowerClamp)
	{
		minIntervalMs = 50;
		minQuietDrift2 = 400.f;
	}
	else if (kind == HeroSnapKind::PeriodicCalmWalk)
	{
		minIntervalMs = 90;
		minQuietDrift2 = 49.f; // ~7 px
	}
	else if (kind == HeroSnapKind::PeriodicSkillCombat)
	{
		// Dash / skill timelines diverge fastest—send more often and on smaller deltas.
		minIntervalMs = 72;
		minQuietDrift2 = 144.f; // ~12 px
	}
	else if (kind == HeroSnapKind::ImmediateBurst)
	{
		minIntervalMs = 0;
		minQuietDrift2 = 0.f;
	}
	else
	{
		minIntervalMs = 220;
		minQuietDrift2 = 100.f; // ~10 px
	}

	if (kind != HeroSnapKind::ImmediateBurst)
	{
		if (msSince < minIntervalMs && drift2 < minQuietDrift2 && s_haveLastSentPos)
			return;
	}
	else if (drift2 < 0.01f && s_haveLastSentPos && msSince < 16)
	{
		return;
	}

	char heroSnapBuf[192];
	std::snprintf(heroSnapBuf, sizeof(heroSnapBuf),
				  "{\"type\":\"hero_snap\",\"x\":%.2f,\"y\":%.2f,\"ts\":%ld}",
				  (double)worldPos.x, (double)worldPos.y, (long)time(nullptr));
	GamePlatformWsSend(heroSnapBuf);
	s_lastSentPos = worldPos;
	s_haveLastSentPos = true;
	s_lastSentAt = now;
#else
	(void)worldPos;
	(void)kind;
#endif
}

void GameLayer::tickOnlineHeroPositionSnap(float)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (!_isStarted || _isExiting || !GamePlatformWsConnected() || !currentPlayer ||
		currentPlayer->getState() == State::DEAD)
		return;
	const State st = currentPlayer->getState();
	const bool calm = (st == State::IDLE || st == State::WALK);
	sendNetworkOwnedHeroPositionSnapIfNeeded(currentPlayer->getPosition(),
											 calm ? HeroSnapKind::PeriodicCalmWalk
												  : HeroSnapKind::PeriodicSkillCombat);
	syncOnlineBattleStatsToPeer(false);
#endif
}

void BattleRuntimeSystem::onGameStart(GameLayer *layer, bool skipInitFlogs, float flogSpawnDuration) const
{
	if (!layer)
		return;

	layer->_isStarted = true;
	layer->getHudLayer()->openingSprite->removeFromParent();
	layer->getHudLayer()->openingSprite = nullptr;
	layer->schedule(schedule_selector(GameLayer::updateGameTime), 1.0f);
	layer->schedule(schedule_selector(GameLayer::checkBackgroundMusic), 2.0f);
	if (!skipInitFlogs)
	{
		layer->initFlogs();
		layer->_flogWaveSeqCounter = 0;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
		const bool onlineWs = GamePlatformWsConnected();
		const bool flogAuthority = !onlineWs || GetNetworkForcedTeam() == 0;
#else
		const bool flogAuthority = true;
#endif
		if (flogAuthority)
		{
			layer->schedule(schedule_selector(GameLayer::addFlog), flogSpawnDuration);
			layer->addFlog(0);
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
			if (onlineWs && GetNetworkForcedTeam() == 0)
				layer->schedule(schedule_selector(GameLayer::tickOnlineFlogSnap), 0.1f);
#endif
		}
	}

	layer->setKeyEventHandler();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (GamePlatformWsConnected())
	{
		layer->schedule(schedule_selector(GameLayer::tickOnlineHeroPositionSnap), 1.f / 30.f);
	}
#endif
	for (auto hero : layer->_CharacterArray)
	{
		hero->setWalkSpeed(hero->_originSpeed);
		if (hero->isCom())
		{
			const char *remoteHeroName = GetNetworkEnemyHeroName();
			if (!GetNetworkOpponentIsBot() && remoteHeroName && strlen(remoteHeroName) > 0 &&
				hero->getName() == remoteHeroName)
			{
				hero->unschedule(schedule_selector(CharacterBase::setAI));
				hero->_isAI = false;
				continue;
			}
			hero->doAI();
		}
	}
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (GamePlatformWsConnected())
		layer->syncOnlineBattleStatsToPeer(true);
#endif
}

void BattleRuntimeSystem::updateGameTime(GameLayer *layer) const
{
	if (!layer)
		return;

	layer->_second += 1;
	if (layer->_second == 60)
	{
		layer->_minute += 1;
		layer->_second = 0;
	}
	auto tempTime = format("{:02d}:{:02d}", layer->_minute, layer->_second);
	layer->getHudLayer()->gameClock->setString(tempTime.c_str());
	layer->setTotalTime(layer->getTotalTime() + 1);
}

void BattleRuntimeSystem::updateViewPoint(GameLayer *layer) const
{
	if (!layer || !layer->currentPlayer)
		return;

	Vec2 playerPoint;
	if (layer->ougisChar)
		playerPoint = layer->ougisChar->getPosition();
	else if (layer->controlChar)
		playerPoint = layer->controlChar->getPosition();
	else
		playerPoint = layer->currentPlayer->getPosition();

	int x = MAX(playerPoint.x, winSize.width / 2);
	int y = MAX(playerPoint.y, winSize.width / 2);
	x = MIN(x, (layer->currentMap->getMapSize().width * layer->currentMap->getTileSize().width) - winSize.width / 2);
	y = MIN(y, (layer->currentMap->getMapSize().height * layer->currentMap->getTileSize().height) - winSize.height / 2);
	layer->setPosition(Vec2(winSize.width / 2, y) - Vec2(x, y));
}

void SpawnSystem::initMatchUnits(GameLayer *layer) const
{
	if (!layer)
		return;
	layer->initTileMap();
	if (!layer->currentMap)
		return;
	layer->initEffects();
}

GameLayer::GameLayer()
{
	_battleRuntimeSystem = std::make_unique<BattleRuntimeSystem>();
	_spawnSystem = std::make_unique<SpawnSystem>();
	_sessionState = std::make_unique<SessionState>();

	mapId = 0;

	_isAttackButtonRelease = true;
	_isSkillFinish = true;

	_second = 0;
	_minute = 0;
	_playNum = 2;

	kEXPBound = 25;
	aEXPBound = 25;

	_isShacking = false;
	_isSurrender = false;
	_hasSpawnedGuardian = false;
	_guardianPickIdx = -1;

	_isStarted = false;
	_isExiting = false;
	_hasGameOverTriggered = false;

	ougisChar = nullptr;
	controlChar = nullptr;

	_enableGear = true;
	_isOugis2Game = false;
	_isHardCoreGame = false;
	_isRandomChar = false;

	currentPlayer = nullptr;

	_isGear = false;
	_isPause = false;
	_gearLayer = nullptr;
	_pauseLayer = nullptr;
	_gearOpenedWithPushScene = false;
	_pauseOpenedWithPushScene = false;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	_lastPressedMovementKey = -100;
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
	_lastPressedMovementKey = -100;
	_window = GLView::sharedOpenGLView()->m_window;
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	_lastPressedMovementKey = -100;
#endif
}

GameLayer::~GameLayer()
{
	_gLayer = nullptr;
	removeKeyEventHandler();
}

bool GameLayer::init()
{
	Texture2D::setDefaultAlphaPixelFormat(kCCTexture2DPixelFormat_RGBA8888);
	setTouchEnabled(true);

	_gLayer = this;
	const auto &gd = getGameModeHandler()->gd;
	_enableGear = gd.enableGear;
	_isHardCoreGame = gd.isHardCore;
	_isRandomChar = gd.isRandomChar;
	is4V4Mode = gd.use4v4SpawnLayout;
	playerGroup = gd.playerGroup;

	return Layer::init();
}

void GameLayer::onEnter()
{
	if (_isExiting)
	{
		onLeft();
		return;
	}

	if (currentPlayer && !ougisChar)
	{
		if (currentPlayer->getState() == State::WALK)
		{
			currentPlayer->idle();
		}
	}

	Layer::onEnter();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	resetNetworkInputStateOnEnter();
#endif

	if (_isSurrender)
	{
		onGameOver(false);
	}
}

void GameLayer::onExit()
{
	Layer::onExit();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	resetNetworkInputStateOnExit();
#endif

	if (_isExiting)
	{
		_isExiting = false;
	}
}

void GameLayer::onHUDInitialized(const OnHUDInitializedCallback &callback)
{
	callbackssList.push_back(callback);
}

bool GameLayer::isHUDInit()
{
	return isHUDInitialized;
}

void GameLayer::initTileMap()
{
	setRand();
	int mapCount = getMapCount();
	if (mapCount == 0)
	{
		CCMessageBox("Not found any map", "[Error] Not found any map");
		return;
	}
	int forcedMapId = 0;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	forcedMapId = GetNetworkForcedMapId();
#endif
	if (forcedMapId > 0 && forcedMapId <= mapCount)
		mapId = forcedMapId;
	else
		mapId = random(mapCount) + 1;
	currentMap = TMXTiledMap::create(GetMapPath(mapId));
	addChild(currentMap, kMapOrder);
}

void GameLayer::initGard(int guardianVariant, bool notifyNetworkPeers, const std::string *anchorTowerName)
{
	if (_hasSpawnedGuardian)
		return;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	/** Hardcore guardians (Roshi/Han) are disabled for WS matches — avoids sync issues and keeps lanes simpler. */
	if (GamePlatformWsConnected())
		return;
#endif

	setRand();
	const int index = (guardianVariant >= 0) ? (guardianVariant % 2) : random(2);
	auto guardianName = index == 0 ? GuardianEnum::Roshi : GuardianEnum::Han;

	Group guardianGroup = Group::Konoha;
	Vec2 spawnPos(272, 80);

	if (anchorTowerName && !anchorTowerName->empty())
	{
		if (*anchorTowerName == TowerEnum::AkatsukiCenter)
		{
			guardianGroup = Group::Akatsuki;
			spawnPos = Vec2(2800, 80);
		}
		else if (*anchorTowerName == TowerEnum::KonohaCenter)
		{
			guardianGroup = Group::Konoha;
			spawnPos = Vec2(272, 80);
		}
		else
		{
			return;
		}
	}
	else
	{
		guardianGroup = playerGroup == Group::Konoha ? Group::Akatsuki : Group::Konoha;
		if (playerGroup == Group::Konoha)
			spawnPos = Vec2(2800, 80);
		else
			spawnPos = Vec2(272, 80);
	}

	_guardianPickIdx = index;

	auto guardian = Provider::create(guardianName, Role::Com, guardianGroup);

	guardian->setPosition(spawnPos);
	guardian->setSpawnPoint(spawnPos);

	addChild(guardian, -guardian->getPositionY());
	guardian->setLV(6);
	guardian->setHPbar();
	guardian->setShadows();
	guardian->setCharId(_CharacterArray.size() + 1);

	guardian->idle();
	guardian->setSkillEffect("smk");

	guardian->doAI();

	_CharacterArray.push_back(guardian);
	_hudLayer->addMapIcon();

	_hasSpawnedGuardian = true;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (notifyNetworkPeers && GamePlatformWsConnected())
	{
		char buf[224];
		if (anchorTowerName && !anchorTowerName->empty())
			snprintf(buf, sizeof(buf), "{\"type\":\"guardian_spawn\",\"idx\":%d,\"tower\":\"%s\"}", index,
					 anchorTowerName->c_str());
		else
			snprintf(buf, sizeof(buf), "{\"type\":\"guardian_spawn\",\"idx\":%d}", index);
		GamePlatformWsSend(buf);
	}
#else
	(void)notifyNetworkPeers;
#endif
}

void GameLayer::initHeros()
{
	_spawnSystem->initMatchUnits(this);
	if (currentMap == nullptr)
	{
		// initTileMap() failed (e.g. no map asset bundled). The error has
		// already been surfaced to the user via CCMessageBox; bail out here
		// instead of dereferencing the null tilemap below.
		return;
	}

	addSprites("UI/hpBar/hpBar.plist");

	auto handler = getGameModeHandler();
	auto herosDataVector = handler->getHerosArray();

	_isOugis2Game = true;

	TMXObjectGroup *group = currentMap->objectGroupNamed("object");
	if (group == nullptr)
	{
		CCMessageBox("Map is missing the 'object' layer", "[Error] Bad map");
		return;
	}
	CCArray *objectArray = group->getObjects();

	// 4v4 spawn layout
	if (is4V4Mode)
	{
		auto &hero1 = herosDataVector.at(0);
		auto &hero5 = herosDataVector.at(4);

		hero1.setSpawnPoint(getCustomSpawnPoint(hero1));
		addHero(hero1, 1);

		hero5.setSpawnPoint(getCustomSpawnPoint(hero5));
		addHero(hero5, 5);
	}

	int i = 0;
	int konohaSpawnIndex = 0;
	int akatsukiSpawnIndex = 0;
	for (auto &data : herosDataVector)
	{
		if (data.isInit)
			continue;

		int mapPos = 0;
		if (data.group == Group::Akatsuki)
		{
			mapPos = MapPosCount + (akatsukiSpawnIndex % MapPosCount);
			akatsukiSpawnIndex++;
		}
		else
		{
			mapPos = (konohaSpawnIndex % MapPosCount);
			konohaSpawnIndex++;
		}

		Ref *mapObject = objectArray->objectAtIndex(mapPos);
		auto mapdict = (CCDictionary *)mapObject;
		int x = ((CCString *)mapdict->objectForKey("x"))->intValue();
		int y = ((CCString *)mapdict->objectForKey("y"))->intValue();
		data.setSpawnPoint(Vec2(x, y));

		if (is4V4Mode)
		{
			int id = i + 2;
			if (id >= 5)
				id++;
			addHero(data, id);
		}
		else
		{
			addHero(data, i + 1);
		}
		i++;
	}

	// Tower HP bar color depends on currentPlayer group, so towers must be
	// initialized after at least one hero/player is created.
	initTower();

	schedule(schedule_selector(GameLayer::updateViewPoint), 0.00f);
	scheduleOnce(schedule_selector(GameLayer::playGameOpeningAnimation), 0.5f);
}

Hero *GameLayer::addHero(const HeroData &data, int charId)
{
	return addHero(data.name, data.role, data.group, data.spawnPoint, charId);
}

Hero *GameLayer::addHero(const string &name, Role role, Group group, Vec2 spawnPoint, int charId)
{
	auto hero = Provider::create(name, role, group);
	if (hero->isPlayer())
	{
		currentPlayer = hero;
	}
	hero->setPosition(spawnPoint);
	hero->setSpawnPoint(spawnPoint);
	// NOTE: Set all characters speed to zero. (Control movement before game real start)
	hero->setWalkSpeed(0);
	if (group == Group::Akatsuki)
	{
		hero->_isFlipped = true;
		hero->setFlipX(true);
	}
	hero->setHPbar();
	hero->setShadows();
	hero->idle();
	hero->setCharId(charId);
	hero->schedule(schedule_selector(CharacterBase::setRestore2), 1.0f);

	addChild(hero, -hero->getPositionY());
	_CharacterArray.push_back(hero);

	getGameModeHandler()->onCharacterInit(hero);
	return hero;
}

void GameLayer::playGameOpeningAnimation(float dt)
{
	getHudLayer()->playGameOpeningAnimation();

	setRand();
	auto path = random(2) == 0 ? "Audio/Menu/battle_start1.ogg" : "Audio/Menu/battle_start.ogg";
	SimpleAudioEngine::sharedEngine()->playEffect(path);

	scheduleOnce(schedule_selector(GameLayer::onGameStart), 0.75f);
}

void GameLayer::onGameStart(float dt)
{
	auto handler = getGameModeHandler();
	_battleRuntimeSystem->onGameStart(this, handler->skipInitFlogs, handler->flogSpawnDuration);

	getGameModeHandler()->onGameStart();
}

void GameLayer::initFlogs()
{
	addSprites("UI/hpBar/flogBar.plist");

	kName = FlogEnum::KotetsuFlog;
	aName = FlogEnum::FemalePainFlog;
}

void GameLayer::spawnFlogWave(uint32_t waveSeq)
{
	auto KonohaFlogName = kName;
	auto AkatsukiFlogName = aName;

	int i;
	Flog *flog;
	float mainPosY;
	for (i = 0; i < kFlogCount; i++)
	{
		flog = Flog::create();
		flog->setID(KonohaFlogName, Role::Flog, Group::Konoha);
		flog->_netWaveSeq = waveSeq;
		flog->_netSlot = (uint8_t)i;
		if (i < kFlogCount / 2)
			mainPosY = (5.5 - i / 1.5) * 32;
		else
			mainPosY = (3.5 - i / 1.5) * 32;
		flog->_mainPosY = mainPosY;
		flog->setPosition(Vec2(13 * 32, flog->_mainPosY));
		flog->setHPbar();
		flog->idle();
		flog->doAI();
		_KonohaFlogArray.push_back(flog);
		addChild(flog, -int(flog->getPositionY()));
	}

	for (i = 0; i < kFlogCount; i++)
	{
		flog = Flog::create();
		flog->setID(AkatsukiFlogName, Role::Flog, Group::Akatsuki);
		flog->_netWaveSeq = waveSeq;
		flog->_netSlot = (uint8_t)(kFlogCount + i);
		if (i < kFlogCount / 2)
			mainPosY = (5.5 - i / 1.5) * 32;
		else
			mainPosY = (3.5 - i / 1.5) * 32;
		flog->_mainPosY = mainPosY;
		flog->setPosition(Vec2(83 * 32, flog->_mainPosY));
		flog->setHPbar();
		flog->idle();
		flog->doAI();
		_AkatsukiFlogArray.push_back(flog);
		addChild(flog, -flog->getPositionY());
	}
}

void GameLayer::applyPeerFlogWaveFromNetwork(uint32_t waveSeq)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (!GamePlatformWsConnected() || GetNetworkForcedTeam() == 0)
		return;
#endif
	spawnFlogWave(waveSeq);
}

void GameLayer::applyPeerFlogSnapFromNetwork(const std::string &dcsv)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (!GamePlatformWsConnected() || GetNetworkForcedTeam() == 0 || !_isStarted || _isExiting)
		return;
#endif
	std::stringstream ss(dcsv);
	std::string rec;
	while (std::getline(ss, rec, '|'))
	{
		if (rec.empty())
			continue;
		unsigned w = 0;
		unsigned s = 0;
		unsigned hp = 0;
		unsigned st = 0;
		unsigned flip = 0;
		float x = 0.f;
		float y = 0.f;
		int parsed =
			std::sscanf(rec.c_str(), "%u,%u,%f,%f,%u,%u,%u", &w, &s, &x, &y, &hp, &st, &flip);
		if (parsed != 7)
		{
			parsed = std::sscanf(rec.c_str(), "%u,%u,%f,%f,%u,%u", &w, &s, &x, &y, &hp, &st);
			if (parsed != 6)
				continue;
			flip = 0;
		}
		Flog *flog = findFlogByNetworkSlot(this, (uint32_t)w, s);
		if (!flog)
			continue;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
		if (GamePlatformWsConnected() && GetNetworkForcedTeam() != 0)
		{
			const Vec2 tgt(x, y);
			flog->_followNetTargetPos = tgt;
			flog->_followNetSmoothActive = (hp > 0);
			const float dx = tgt.x - flog->getPositionX();
			const float dy = tgt.y - flog->getPositionY();
			if (dx * dx + dy * dy > 360000.f)
				flog->setPosition(tgt);
			reorderChild(flog, -(int)flog->getPositionY());
		}
		else
#endif
		{
			flog->setPosition(Vec2(x, y));
			reorderChild(flog, -(int)flog->getPositionY());
		}
		flog->setHPValue((uint32_t)hp, false);
		if (flog->_hpBar)
			flog->_hpBar->syncVisualPercent(flog->getHpPercent());
		if (hp == 0 && flog->getState() != State::DEAD)
			flog->dead();
		else if (hp > 0)
			flog->syncFollowerMirrorVisual((State)st, flip != 0);
	}
}

void GameLayer::tickOnlineFlogSnap(float)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (!GamePlatformWsConnected() || GetNetworkForcedTeam() != 0 || !_isStarted || _isExiting || !currentMap)
		return;
	std::string d;
	d.reserve(4096);
	for (auto *f : _KonohaFlogArray)
	{
		if (!f)
			continue;
		char chunk[128];
		std::snprintf(chunk, sizeof(chunk), "%u,%u,%.2f,%.2f,%u,%u,%u|", (unsigned)f->_netWaveSeq, (unsigned)f->_netSlot,
					  f->getPositionX(), f->getPositionY(), (unsigned)f->getHP(), (unsigned)f->getState(),
					  f->_isFlipped ? 1u : 0u);
		d += chunk;
	}
	for (auto *f : _AkatsukiFlogArray)
	{
		if (!f)
			continue;
		char chunk[128];
		std::snprintf(chunk, sizeof(chunk), "%u,%u,%.2f,%.2f,%u,%u,%u|", (unsigned)f->_netWaveSeq, (unsigned)f->_netSlot,
					  f->getPositionX(), f->getPositionY(), (unsigned)f->getHP(), (unsigned)f->getState(),
					  f->_isFlipped ? 1u : 0u);
		d += chunk;
	}
	if (!d.empty())
		d.pop_back();
	if (d.empty())
		return;
	std::string json = "{\"type\":\"flog_snap\",\"d\":\"";
	json += d;
	json += "\"}";
	GamePlatformWsSend(json.c_str());
#endif
}

void GameLayer::addFlog(float dt)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (GamePlatformWsConnected() && GetNetworkForcedTeam() != 0)
		return;
#endif
	const uint32_t waveSeq = _flogWaveSeqCounter++;
	spawnFlogWave(waveSeq);
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (GamePlatformWsConnected() && GetNetworkForcedTeam() == 0)
	{
		char buf[160];
		std::snprintf(buf, sizeof(buf),
					  "{\"type\":\"flog_wave\",\"seq\":%u,\"ts\":%ld}",
					  waveSeq, (long)std::time(nullptr));
		GamePlatformWsSend(buf);
	}
#endif
}

void GameLayer::initTower()
{
	addSprites(format("Unit/Tower/Tower{}.plist", mapId));

	TMXObjectGroup *metaGroup = currentMap->objectGroupNamed("meta");
	CCArray *metaArray = metaGroup->getObjects();
	Ref *pObject;
	int i = 0;

	CCARRAY_FOREACH(metaArray, pObject)
	{
		auto dict = (CCDictionary *)pObject;

		int metaX = ((CCString *)dict->objectForKey("x"))->intValue();
		int metaY = ((CCString *)dict->objectForKey("y"))->intValue();

		int metaWidth = ((CCString *)dict->objectForKey("width"))->intValue();
		int metaHeight = ((CCString *)dict->objectForKey("height"))->intValue();

		auto name = ((CCString *)dict->objectForKey("name"))->m_sString;

		Tower *tower = Tower::create();
		char towerName[7] = "abcdef";
		strncpy(towerName, name.c_str(), 6);
		if (is_same(towerName, kGroupKonoha))
		{
			tower->setID(name, Role::Tower, Group::Konoha);
		}
		else
		{
			tower->setID(name, Role::Tower, Group::Akatsuki);
			tower->setFlipX(true);
			tower->_isFlipped = true;
		}
		float posX = metaX + metaWidth / 2;
		float posY = metaY + metaHeight / 2;
		tower->setPosition(Vec2(posX, posY));
		tower->setSpawnPoint(Vec2(posX, posY));
		tower->setCharId(i + 1);

		if (i == 1 || i == 4)
		{
			if (is4V4Mode)
			{
				tower->setMaxHPValue(80000, false);
			}
			else
			{
				tower->setMaxHPValue(50000, false);
			}
			tower->setHPValue(tower->getMaxHP(), false);
		}
		tower->setHPbar();
		tower->_hpBar->setVisible(false);
		tower->idle();
		addChild(tower, -tower->getPositionY());

		_TowerArray.push_back(tower);
		i++;
	}
}

void GameLayer::initEffects()
{
	addSprites("Effects/SkillEffect.plist");
	skillEffectBatch = SpriteBatchNode::create("Effects/SkillEffect.png");
	addChild(skillEffectBatch, kSkillEffectOrder);

	addSprites("Effects/DamageEffect.plist");
	damageEffectBatch = SpriteBatchNode::create("Effects/DamageEffect.png");
	addChild(damageEffectBatch, kDamageEffectOrder);

	addSprites("Effects/Shadows.plist");
	shadowBatch = SpriteBatchNode::create("Effects/Shadows.png");
	addChild(shadowBatch, kShadowOrder);
}

void GameLayer::updateGameTime(float dt)
{
	_battleRuntimeSystem->updateGameTime(this);
}

void GameLayer::updateViewPoint(float dt)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	processRemoteInputQueue(this);
#endif
	_battleRuntimeSystem->updateViewPoint(this);
}

void GameLayer::setTowerState(int charId)
{
	_hudLayer->setTowerState(charId);
}

void GameLayer::updateHudSkillButtons()
{
	_hudLayer->updateSkillButtons();
}

void GameLayer::setHPLose(float percent)
{
	_hudLayer->setHPLose(percent);
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (GamePlatformWsConnected())
		syncOnlineBattleStatsToPeer(false);
#endif
}

void GameLayer::syncOnlineBattleStatsToPeer(bool force)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	static std::chrono::steady_clock::time_point s_lastBattleStatSent{};
	static bool s_didSendBattleStat = false;
	static constexpr int kBattleStatMinIntervalMs = 180;

	if (!GamePlatformWsConnected() || !_isStarted || _isExiting || !currentPlayer)
		return;

	const auto now = std::chrono::steady_clock::now();
	if (!force && s_didSendBattleStat)
	{
		const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastBattleStatSent).count();
		if (dt < kBattleStatMinIntervalMs)
			return;
	}
	s_didSendBattleStat = true;
	s_lastBattleStatSent = now;

	const uint32_t hp = currentPlayer->getHP();
	const uint32_t maxHp = currentPlayer->getMaxHP();
	const uint32_t kills = currentPlayer->getKillNum();
	const uint32_t deaths = currentPlayer->_deadNum;
	const uint32_t flog = currentPlayer->_flogNum;
	int kono = 0;
	int aka = 0;
	if (_hudLayer && _hudLayer->KonoLabel && _hudLayer->AkaLabel)
	{
		kono = to_int(_hudLayer->KonoLabel->getString());
		aka = to_int(_hudLayer->AkaLabel->getString());
	}

	char buf[360];
	std::snprintf(buf, sizeof(buf),
				  "{\"type\":\"battle_stat\",\"hp\":%u,\"maxHp\":%u,\"kills\":%u,\"deaths\":%u,\"flog\":%u,"
				  "\"kono\":%d,\"aka\":%d,\"ts\":%ld}",
				  hp, maxHp, kills, deaths, flog, kono, aka, (long)std::time(nullptr));
	GamePlatformWsSend(buf);
#else
	(void)force;
#endif
}

void GameLayer::setCKRLose(bool isCRK2)
{
	_hudLayer->setCKRLose(isCRK2);
}

void GameLayer::setReport(const string &slayer, const string &dead, uint32_t killNum)
{
	_hudLayer->setReport(slayer, dead, killNum);
}

void GameLayer::resetStatusBar()
{
	_hudLayer->status_hpbar->setRotation(0);
}

void GameLayer::setCoin(const char *value)
{
	_hudLayer->setCoin(value);
}

void GameLayer::removeOugisMark(int type)
{
	if (type == 1)
	{
		if (_hudLayer->skill4Button)
		{
			if (_hudLayer->skill4Button->lockLabel1)
			{
				_hudLayer->skill4Button->lockLabel1->removeFromParent();
				_hudLayer->skill4Button->lockLabel1 = nullptr;
			}
		}
	}
	else
	{
		if (_hudLayer->skill5Button)
		{
			if (_hudLayer->skill5Button->lockLabel1)
			{
				_hudLayer->skill5Button->lockLabel1->removeFromParent();
				_hudLayer->skill5Button->lockLabel1 = nullptr;
			}
		}
	}
}

void GameLayer::checkTower()
{
	int konohaTowerCount = 0;
	int akatsukiTowerCount = 0;

	for (auto tower : _TowerArray)
	{
		if (tower->isKonohaGroup())
			konohaTowerCount++;
		else
			akatsukiTowerCount++;
	}

	if (konohaTowerCount == 2)
	{
		aName = FlogEnum::PainFlog;
		kEXPBound = 50;
	}
	else if (konohaTowerCount == 1)
	{
		aName = FlogEnum::ObitoFlog;
		kEXPBound = 100;
	}

	if (akatsukiTowerCount == 2)
	{
		kName = FlogEnum::IzumoFlog;
		aEXPBound = 50;
	}
	else if (akatsukiTowerCount == 1)
	{
		kName = FlogEnum::KakashiFlog;
		aEXPBound = 100;
	}

	for (auto hero : getGameLayer()->_CharacterArray)
	{
		if (hero->isNotCom())
			continue;

		if (hero->isKonohaGroup())
		{
			hero->battleCondiction = konohaTowerCount - akatsukiTowerCount;
			if (konohaTowerCount == 1)
			{
				hero->isBaseDanger = true;
			}
		}
		else
		{
			hero->battleCondiction = akatsukiTowerCount - konohaTowerCount;
			if (_isHardCoreGame)
			{
				if (akatsukiTowerCount == 1)
				{
					hero->isBaseDanger = true;
				}
			}
		}
	}

	if (konohaTowerCount == 0 || akatsukiTowerCount == 0)
	{
		if (playerGroup == Group::Konoha)
			onGameOver(konohaTowerCount != 0);
		else
			onGameOver(akatsukiTowerCount != 0);
	}
}

void GameLayer::clearDoubleClick()
{
	if (_hudLayer->skill1Button->getDoubleSkill() &&
		_hudLayer->skill1Button->_clickNum >= 1)
	{
		_hudLayer->skill1Button->setFreezeAction(nullptr);
		_hudLayer->skill1Button->beganAnimation();
	}
}

void GameLayer::onPause()
{
	if (_isPause)
		return;

	sendNetworkMatchUIIfActive("pause", true);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (_isStarted && GamePlatformWsConnected())
	{
		sendNetworkJoyReleaseEvent();
		if (currentPlayer && currentPlayer->getState() == State::WALK)
			currentPlayer->idle();
	}
#endif

	_isPause = true;
	bool isNetworkOverlay = false;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	isNetworkOverlay = _isStarted && GamePlatformWsConnected();
#endif
	Scene *f = Director::sharedDirector()->getRunningScene();
	RenderTexture *snapshoot = nullptr;
	if (!isNetworkOverlay)
	{
		snapshoot = RenderTexture::create(winSize.width, winSize.height);
		Ref *pObject = f->getChildren()->objectAtIndex(0);
		BGLayer *bg = (BGLayer *)pObject;
		snapshoot->begin();
		bg->visit();

		visit();
		snapshoot->end();
	}

	PauseLayer *layer = PauseLayer::create(snapshoot, isNetworkOverlay);
	_pauseLayer = layer;
	if (isNetworkOverlay)
	{
		f->addChild(layer, 5000);
		_pauseOpenedWithPushScene = false;
	}
	else
	{
		Scene *pscene = Scene::create();
		pscene->addChild(layer);
		Director::sharedDirector()->pushScene(pscene);
		_pauseOpenedWithPushScene = true;
	}
}

void GameLayer::resumeFromPause()
{
	if (!_isPause)
		return;

	if (_pauseOpenedWithPushScene)
	{
		if (UserDefault::sharedUserDefault()->getBoolForKey("isBGM"))
		{
			SimpleAudioEngine::sharedEngine()->resumeBackgroundMusic();
		}
		if (UserDefault::sharedUserDefault()->getBoolForKey("isVoice"))
		{
			SimpleAudioEngine::sharedEngine()->resumeAllEffects();
		}
		_pauseLayer = nullptr;
		Director::sharedDirector()->popScene();
		_pauseOpenedWithPushScene = false;
	}
	else if (_pauseLayer && _pauseLayer->getParent())
	{
		_pauseLayer->removeFromParent();
		_pauseLayer = nullptr;
	}
	else
	{
		if (UserDefault::sharedUserDefault()->getBoolForKey("isBGM"))
		{
			SimpleAudioEngine::sharedEngine()->resumeBackgroundMusic();
		}
		if (UserDefault::sharedUserDefault()->getBoolForKey("isVoice"))
		{
			SimpleAudioEngine::sharedEngine()->resumeAllEffects();
		}
		Director::sharedDirector()->popScene();
	}
	_isPause = false;
	sendNetworkMatchUIIfActive("pause", false);
}

void GameLayer::onGear()
{
	if (!_enableGear)
		return;
	if (_isGear)
		return;

	sendNetworkMatchUIIfActive("gear", true);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (_isStarted && GamePlatformWsConnected())
	{
		sendNetworkJoyReleaseEvent();
		if (currentPlayer && currentPlayer->getState() == State::WALK)
			currentPlayer->idle();
	}
#endif

	_isGear = true;
	bool isNetworkOverlay = false;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	isNetworkOverlay = _isStarted && GamePlatformWsConnected();
#endif

	Scene *f = Director::sharedDirector()->getRunningScene();
	RenderTexture *snapshoot = nullptr;
	if (!isNetworkOverlay)
	{
		snapshoot = RenderTexture::create(winSize.width, winSize.height);
		Ref *pObject = f->getChildren()->objectAtIndex(0);
		BGLayer *bg = (BGLayer *)pObject;
		snapshoot->begin();
		bg->visit();

		visit();
		snapshoot->end();
	}

	GearLayer *layer = GearLayer::create(snapshoot, isNetworkOverlay);
	_gearLayer = layer;
	layer->updatePlayerGear();
	if (isNetworkOverlay)
	{
		f->addChild(layer, 5000);
		_gearOpenedWithPushScene = false;
	}
	else
	{
		Scene *pscene = Scene::create();
		pscene->addChild(layer);
		Director::sharedDirector()->pushScene(pscene);
		_gearOpenedWithPushScene = true;
	}
}

void GameLayer::dismissGearOverlay()
{
	if (!_isGear)
		return;
	if (_hudLayer)
	{
		_hudLayer->updateGears();
	}
	if (_gearOpenedWithPushScene)
	{
		_gearLayer = nullptr;
		Director::sharedDirector()->popScene();
		_gearOpenedWithPushScene = false;
	}
	else if (_gearLayer && _gearLayer->getParent())
	{
		_gearLayer->removeFromParent();
		_gearLayer = nullptr;
	}
	else
	{
		Director::sharedDirector()->popScene();
	}
	_isGear = false;
	sendNetworkMatchUIIfActive("gear", false);
}

void GameLayer::onGameOver(bool isWin)
{
	if (_hasGameOverTriggered)
		return;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	if (GamePlatformWsConnected())
		syncOnlineBattleStatsToPeer(true);
#endif

	_hasGameOverTriggered = true;
	_isStarted = false;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	unschedule(schedule_selector(GameLayer::tickOnlineHeroPositionSnap));
	unschedule(schedule_selector(GameLayer::tickOnlineFlogSnap));
#endif

	bool isApplyingRemoteMatchEnd = false;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	isApplyingRemoteMatchEnd = s_isApplyingRemoteMatchEnd;
#endif
	if (!isApplyingRemoteMatchEnd)
	{
		sendNetworkMatchEnd(isWin, _isSurrender ? "surrender" : "game_over");
	}

	removeKeyEventHandler();

	if (_isPause)
	{
		resumeFromPause();
	}
	if (_isGear)
	{
		dismissGearOverlay();
	}

	RenderTexture *snapshoot = RenderTexture::create(winSize.width, winSize.height);
	Scene *f = Director::sharedDirector()->getRunningScene();
	Ref *pObject = f->getChildren()->objectAtIndex(0);
	BGLayer *bg = (BGLayer *)pObject;
	snapshoot->begin();
	bg->visit();
	visit();
	snapshoot->end();

	getGameModeHandler()->Internal_GameOver();

	Scene *pscene = Scene::create();
	GameOver *layer = GameOver::create(snapshoot);
	layer->setWin(isWin);
	pscene->addChild(layer);
	Director::sharedDirector()->pushScene(pscene);
}

void GameLayer::onLeft()
{
	CCNotificationCenter::sharedNotificationCenter()->purgeNotificationCenter();

	CCArray *childArray = getChildren();
	Ref *pObject;
	CCARRAY_FOREACH(childArray, pObject)
	{
		auto ac = (Node *)pObject;
		ac->unscheduleUpdate();
		ac->unscheduleAllSelectors();
	}

	LoadLayer::unloadAllCharsIMG(_CharacterArray);
	removeSprites(format("Unit/Tower/Tower{}.plist", mapId));

	if (_isHardCoreGame)
	{
		removeSprites(kGuardian_Han);
		removeSprites(kGuardian_Roshi);
		KTools::prepareFileOGG(GuardianEnum::Han, true);
		KTools::prepareFileOGG(GuardianEnum::Roshi, true);
	}

	KTools::prepareFileOGG("Effect", true);
	KTools::prepareFileOGG("Ougis", true);

	_CharacterArray.clear();
	_TowerArray.clear();
	_KonohaFlogArray.clear();
	_AkatsukiFlogArray.clear();

	removeSprites("UI.plist");
	removeSprites("Map.plist");

	SimpleAudioEngine::sharedEngine()->end();

	lua_call_func(UiFlowKeys::kOnGameOver);
}

void GameLayer::checkBackgroundMusic(float dt)
{
	if (UserDefault::sharedUserDefault()->getBoolForKey("isBGM"))
	{
		if (!SimpleAudioEngine::sharedEngine()->isBackgroundMusicPlaying())
		{
			if (!_isHardCoreGame)
			{
				SimpleAudioEngine::sharedEngine()->playBackgroundMusic(BATTLE_MUSIC);
			}
			else
			{
				int id = (mapId - 1) > 4 ? 4 : (mapId - 1);
				if (_playNum == 0)
				{
					SimpleAudioEngine::sharedEngine()->playBackgroundMusic(format("Audio/Music/Battle{}.ogg", 2 + id * 3).c_str(), false);
					_playNum++;
				}
				else if (_playNum == 1)
				{
					SimpleAudioEngine::sharedEngine()->playBackgroundMusic(format("Audio/Music/Battle{}.ogg", 3 + id * 3).c_str(), false);
					_playNum++;
				}
				else if (_playNum == 2)
				{
					SimpleAudioEngine::sharedEngine()->playBackgroundMusic(format("Audio/Music/Battle{}.ogg", 1 + id * 3).c_str(), false);
					_playNum = 0;
				}
			}
		}
	}
}

void GameLayer::setOugis(CharacterBase *sender)
{
	if (!_hudLayer->ougisLayer)
	{
		ougisChar = sender;

		CCArray *childArray = getChildren();
		Ref *pObject;
		CCARRAY_FOREACH(childArray, pObject)
		{
			auto object = (Node *)pObject;
			object->pauseSchedulerAndActions();
		}
		pauseSchedulerAndActions();

		updateViewPoint(0.01f);

		blend = LayerColor::create(ccc4(0, 0, 0, 200), winSize.width, winSize.height);
		blend->setPosition(Vec2(-getPositionX(), 0));
		addChild(blend, 1000);
		sender->setZOrder(2000);

		if (UserDefault::sharedUserDefault()->getBoolForKey("isVoice"))
		{
			SimpleAudioEngine::sharedEngine()->playEffect(format("Audio/Ougis/{}_ougis.ogg", ougisChar->getName()).c_str());
		}

		_hudLayer->setOugis(ougisChar->getName(), ougisChar->getGroup());
	}
}

void GameLayer::removeOugis()
{
	ougisChar->setZOrder(-ougisChar->getPositionY());
	CCArray *childArray = getChildren();
	Ref *pObject;
	CCARRAY_FOREACH(childArray, pObject)
	{
		auto object = (Node *)pObject;
		object->resumeSchedulerAndActions();
	}
	resumeSchedulerAndActions();

	blend->removeFromParent();
	ougisChar = nullptr;
}

// Input/key control split out for readability.
#include "GameLayerInputControl.inl"

int GameLayer::getMapCount()
{
	int index = 1;
	int mapCount = 0;
	auto fileUtils = FileUtils::sharedFileUtils();
	while (fileUtils->isFileExist(format("Maps/{}.tmx", index++).c_str()))
		mapCount++;
	CCLOG("===== Found %d maps =====", mapCount);
	return mapCount;
}

void GameLayer::invokeAllCallbacks()
{
	isHUDInitialized = true;
	if (callbackssList.size() > 0)
	{
		for (auto &callback : callbackssList)
			callback();
		callbackssList.clear();
	}
}

Vec2 GameLayer::getCustomSpawnPoint(HeroData &data)
{
	data.isInit = true;
	return data.group == Group::Konoha ? Vec2(432, 80) : Vec2(2608, 80);
}

void GameLayer::clearAllFlogsMainTarget(CharacterBase *target)
{
	UnitEx::clearMainTarget(target, _KonohaFlogArray);
	UnitEx::clearMainTarget(target, _AkatsukiFlogArray);
}

void GameLayer::clearAllUnitsMainTarget(CharacterBase *target)
{
	clearAllFlogsMainTarget(target);
	UnitEx::clearMainTarget(target, _AkatsukiFlogArray);
}

