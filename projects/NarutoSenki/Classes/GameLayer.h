#pragma once
#include "GameOver.h"
#include "GearLayer.h"
#include "PauseLayer.h"
#include "Data/UnitData.h"
#include <memory>
#include <string>

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include "glfw3.h"
#include <windows.h>
#define _isPressed(vk_code) (GetAsyncKeyState(vk_code) & 0x8000 ? 1 : 0)
#define isKeyDown(vk_code) (GetAsyncKeyState(vk_code) & 0x8000 ? 1 : 0)
#define getKeyUp(vk_code) (GetAsyncKeyState(vk_code) & 0x8000 ? 0 : 1)
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
#if __has_include("glfw3.h")
#include "glfw3.h"
#elif __has_include(<GLFW/glfw3.h>)
#include <GLFW/glfw3.h>
#elif __has_include("glfw3/include/mac/glfw3.h")
#include "glfw3/include/mac/glfw3.h"
#else
#error "GLFW header not found. Check include paths."
#endif
#define _isPressed(__WINDOW__, __KEY__) glfwGetKey(__WINDOW__, __KEY__)
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include "../../../cocos2dx/platform/android/jni/JniHelper.h"
#endif

enum class HeroSnapKind : uint8_t
{
	TowerClamp,
	PeriodicCalmWalk,
	PeriodicSkillCombat,
	/** One-shot after attack/skill replication—no throttle (drops duplicate coordinates via last-send cache only). */
	ImmediateBurst,
};

class BGLayer;
class CharacterBase;
class Hero;
class Flog;
class Tower;
class GameLayer;
class HudLayer;
class BattleRuntimeSystem;
class SpawnSystem;
struct SessionState;

extern GameLayer *_gLayer;
extern bool _isFullScreen;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX || CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
static GLFWwindow *_window = nullptr;
#endif

inline GameLayer *getGameLayer()
{
	return _gLayer;
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
/** True while applying summon_death from the peer so CharacterBase::dead() does not echo back. */
bool gameLayerIsApplyingPeerSummonDeathFromNetwork();
/** True while applying tower_destroy from the peer so CharacterBase::dead() does not echo WS again. */
bool gameLayerIsApplyingPeerTowerDestroyFromNetwork();
#endif

class GameLayer : public Layer
{
	using OnHUDInitializedCallback = std::function<void()>;

	friend class LoadLayer;
	friend class BattleRuntimeSystem;

public:
	GameLayer();
	~GameLayer();

	TMXTiledMap *currentMap;
	CharacterBase *currentPlayer;

	uint32_t _second;
	uint32_t _minute;
	int mapId;

	const char *kName;
	const char *aName;
	int kEXPBound;
	int aEXPBound;

	bool _isAttackButtonRelease;
	bool _hasSpawnedGuardian;
	/** Roshi/Han pick from last initGard (for WS tower_destroy / guardian sync). */
	int _guardianPickIdx;
	// int _guardianNum;
	vector<Flog *> _KonohaFlogArray;
	vector<Flog *> _AkatsukiFlogArray;
	vector<Tower *> _TowerArray;
	vector<Hero *> _CharacterArray;

	bool _isShacking;

	int _playNum;
	void checkBackgroundMusic(float dt);

	PROP(HudLayer *, _hudLayer, HudLayer);
	void onHUDInitialized(const OnHUDInitializedCallback &callback);
	bool isHUDInit();
	void setTowerState(int charId);

	PROP_UInt(totalKills, TotalKills);
	PROP_UInt(totalTime, TotalTime);

	SpriteBatchNode *skillEffectBatch;
	SpriteBatchNode *damageEffectBatch;
	SpriteBatchNode *bulletBatch;
	SpriteBatchNode *shadowBatch;

	bool init();
	void initTileMap();
	void initHeros();
	void initFlogs();
	/** Online follower only: spawn one frog wave when Konoha peer sends flog_wave. */
	void applyPeerFlogWaveFromNetwork(uint32_t waveSeq);
	/** Online follower (Akatsuki): apply authoritative frog pose/HP CSV from Konoha (see flog_snap). */
	void applyPeerFlogSnapFromNetwork(const std::string &dcsv);
	void initTower();
	/** @param guardianVariant Roshi vs Han: 0 or 1; -1 picks randomly (deterministic when synced online).
	 * @param anchorTowerName When set (KonohaCenter / AkatsukiCenter), spawn uses world placement for that base — same on both peers (fixes online flip). */
	void initGard(int guardianVariant = -1, bool notifyNetworkPeers = true, const std::string *anchorTowerName = nullptr);
	void initEffects();

	void updateViewPoint(float dt);
	void updateGameTime(float dt);

	Hero *addHero(const HeroData &data, int charId);
	Hero *addHero(const string &name, Role role, Group group, Vec2 spawnPoint, int charNo);
	void addFlog(float dt);

	void attackButtonClick(ABType type);
	void gearButtonClick(GearType type);
	void attackButtonRelease();

	void JoyStickRelease();
	void JoyStickUpdate(Vec2 direction);

	PROP(bool, _isSkillFinish, SkillFinish);
	void checkTower();

	void onPause();
	void resumeFromPause();
	void onGear();
	/** Pop gear shop scene; used locally and when mirroring remote close in network play. */
	void dismissGearOverlay();
	/** Online in-match pause/gear menu: suppress replication so stray touches/keys don't confuse the opponent. */
	bool shouldBlockNetworkBattleInputEcho() const;
	/** Broadcast local hero world position—tower edge, calm idle/walk (~4–5 Hz), or skill combat (~13 Hz capped). */
	void sendNetworkOwnedHeroPositionSnapIfNeeded(const Vec2 &worldPos, HeroSnapKind kind);
	/** Scheduled ~10 Hz; internal throttling reduces bandwidth while skills are active. */
	void tickOnlineHeroPositionSnap(float dt);
	/** Konoha authority: ~10 Hz broadcast frog pose + HP so follower mirrors realtime (no ghost hits). */
	void tickOnlineFlogSnap(float dt);
	/** Online: push HP / kill / death stats to peer (throttled unless force). */
	void syncOnlineBattleStatsToPeer(bool force = false);

	void playGameOpeningAnimation(float dt);
	void onGameStart(float dt);
	void onGameOver(bool isWin);

	void updateHudSkillButtons();
	void setHPLose(float percent);
	void setCKRLose(bool isCRK2);

	void setReport(const string &slayer, const string &dead, uint32_t killNum);
	void clearDoubleClick();
	void resetStatusBar();
	void setCoin(const char *value);
	void removeOugisMark(int type);
	void setOugis(CharacterBase *sender);
	void removeOugis();

	CharacterBase *ougisChar;
	CharacterBase *controlChar;
	Layer *blend;

	void onLeft();

	bool _isSurrender;

	bool _enableGear;
	bool _isOugis2Game;
	bool _isHardCoreGame;
	bool _isRandomChar;

	Group playerGroup;
	bool _isStarted;
	bool _isExiting;
	bool _hasGameOverTriggered;

	const char *getGuardianGroup()
	{
		return playerGroup == Group::Konoha ? TowerEnum::AkatsukiCenter : TowerEnum::KonohaCenter;
	}

	bool _isGear;
	bool _isPause;
	GearLayer *_gearLayer;
	PauseLayer *_pauseLayer;
	/** Local/offline: gear UI uses pushScene; online Mac overlay uses addChild on running scene. */
	bool _gearOpenedWithPushScene;
	/** Same distinction as gear for pause menu. */
	bool _pauseOpenedWithPushScene;

	void clearAllFlogsMainTarget(CharacterBase *target);
	void clearAllUnitsMainTarget(CharacterBase *target);

	CREATE_FUNC(GameLayer);
	static bool checkHasAnyMovement();
	static int getMapCount();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX || CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	static void keyEventHandle(GLFWwindow *window, int key, int scancode, int action, int modes);
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	static void keyEventHandle(int key, int keyState);
#endif

private:
	void onEnter();
	void onExit();

	void setKeyEventHandler();
	void removeKeyEventHandler();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX || CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	int _lastPressedMovementKey;
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	int _lastPressedMovementKey;
#endif
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	static void LPFN_ACCELEROMETER_KEYHOOK(UINT message, WPARAM wParam, LPARAM lParam);
#endif

	void invokeAllCallbacks();

	void spawnFlogWave(uint32_t waveSeq);

	inline Vec2 getCustomSpawnPoint(HeroData &data);

	bool isHUDInitialized = false;
	bool is4V4Mode = false;
	vector<OnHUDInitializedCallback> callbackssList;

	std::unique_ptr<BattleRuntimeSystem> _battleRuntimeSystem;
	std::unique_ptr<SpawnSystem> _spawnSystem;
	std::unique_ptr<SessionState> _sessionState;

	/** Increments each spawned frog wave (offline + online); tags frogs for network CSV sync. */
	uint32_t _flogWaveSeqCounter = 0;
};

#define BIND(funcName) std::bind(&funcName, this)
