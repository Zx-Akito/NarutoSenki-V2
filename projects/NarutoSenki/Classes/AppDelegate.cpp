#include "AppDelegate.h"
#include "CCLuaEngine.h"
#include "script_support/CCScriptSupport.h"

#include "Systems/Initializer.hpp"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
#include <CoreFoundation/CoreFoundation.h>
#include <limits.h>
extern "C" {
void MacWsSetEventCallback(void (*callback)(const char *eventName, const char *payload));
bool MacWsConnect(const char *url);
void MacWsSend(const char *message);
void MacWsDisconnect();
bool MacWsIsConnected();
}
#endif

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include "../../../cocos2dx/platform/android/jni/JniHelper.h"
#include <jni.h>
#endif

using NativeWsEventCallback = void (*)(const char *eventName, const char *payload);
static NativeWsEventCallback s_nativeWsEventCallback = nullptr;
static int s_networkForcedMapId = 0;
static int s_networkForcedTeam = -1; // 0 = Konoha, 1 = Akatsuki
static std::string s_networkYourHeroName;
static std::string s_networkEnemyHeroName;
static std::string s_networkYourTeamCsv;
static std::string s_networkEnemyTeamCsv;
static bool s_networkOpponentIsBot = false;

extern "C" void SetNativeWsEventCallback(NativeWsEventCallback callback)
{
	s_nativeWsEventCallback = callback;
}

extern "C" int GetNetworkForcedMapId()
{
	return s_networkForcedMapId;
}

extern "C" int GetNetworkForcedTeam()
{
	return s_networkForcedTeam;
}

extern "C" const char *GetNetworkYourHeroName()
{
	return s_networkYourHeroName.c_str();
}

extern "C" const char *GetNetworkEnemyHeroName()
{
	return s_networkEnemyHeroName.c_str();
}

extern "C" const char *GetNetworkYourTeamCsv()
{
	return s_networkYourTeamCsv.c_str();
}

extern "C" const char *GetNetworkEnemyTeamCsv()
{
	return s_networkEnemyTeamCsv.c_str();
}

extern "C" bool GetNetworkOpponentIsBot()
{
	return s_networkOpponentIsBot;
}

namespace
{
static void sendLuaEvent(const char *eventName, const std::string &payload)
{
	auto engine = LuaEngine::defaultEngine();
	if (!engine || !engine->getLuaStack())
		return;

	auto *L = engine->getLuaStack()->getLuaState();
	lua_getglobal(L, "onWebSocketEvent");
	if (!lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return;
	}

	lua_pushstring(L, eventName);
	lua_pushstring(L, payload.c_str());
	if (lua_pcall(L, 2, 0, 0) != LUA_OK)
	{
		const char *error = lua_tostring(L, -1);
		CCLOG("[WebSocket] lua callback error: %s", error ? error : "<unknown>");
		lua_pop(L, 1);
	}
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
static bool callAndroidStaticVoid(const char *methodName, const char *signature, const char *arg)
{
	JniMethodInfo methodInfo;
	if (!JniHelper::getStaticMethodInfo(methodInfo,
										"re/naruto/game/NarutoSenki",
										methodName,
										signature))
	{
		CCLOG("[WebSocket] missing Java static method: %s", methodName ? methodName : "<null>");
		return false;
	}

	jstring jArg = methodInfo.env->NewStringUTF(arg ? arg : "");
	methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID, jArg);
	methodInfo.env->DeleteLocalRef(jArg);
	methodInfo.env->DeleteLocalRef(methodInfo.classID);
	return true;
}
#endif

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
static void onMacWsEvent(const char *eventName, const char *payload)
{
	sendLuaEvent(eventName ? eventName : "",
				 payload ? payload : "");
	if (s_nativeWsEventCallback)
	{
		s_nativeWsEventCallback(eventName ? eventName : "",
								 payload ? payload : "");
	}
}
#endif

static int luaWsConnect(lua_State *L)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	const char *url = lua_tostring(L, 1);
	const bool ok = MacWsConnect(url ? url : "");
	lua_pushboolean(L, ok ? 1 : 0);
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	const char *url = lua_tostring(L, 1);
	JniMethodInfo methodInfo;
	if (!JniHelper::getStaticMethodInfo(methodInfo,
										"re/naruto/game/NarutoSenki",
										"wsConnect",
										"(Ljava/lang/String;)Z"))
	{
		CCLOG("[WebSocket] wsConnect unavailable: Java method not found");
		lua_pushboolean(L, 0);
		return 1;
	}
	jstring jUrl = methodInfo.env->NewStringUTF(url ? url : "");
	const jboolean result = methodInfo.env->CallStaticBooleanMethod(methodInfo.classID, methodInfo.methodID, jUrl);
	methodInfo.env->DeleteLocalRef(jUrl);
	methodInfo.env->DeleteLocalRef(methodInfo.classID);
	lua_pushboolean(L, result ? 1 : 0);
#else
	(void)lua_tostring(L, 1);
	CCLOG("[WebSocket] wsConnect unavailable: this build has no websocket client");
	lua_pushboolean(L, 0);
#endif
	return 1;
}

static int luaWsSend(lua_State *L)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	const char *message = lua_tostring(L, 1);
	MacWsSend(message ? message : "");
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	const char *message = lua_tostring(L, 1);
	callAndroidStaticVoid("wsSend", "(Ljava/lang/String;)V", message ? message : "");
#else
	(void)lua_tostring(L, 1);
	CCLOG("[WebSocket] wsSend ignored: websocket client unavailable");
#endif
	return 0;
}

static int luaWsDisconnect(lua_State *L)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	MacWsDisconnect();
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	JniMethodInfo methodInfo;
	if (JniHelper::getStaticMethodInfo(methodInfo,
									   "re/naruto/game/NarutoSenki",
									   "wsDisconnect",
									   "()V"))
	{
		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID);
		methodInfo.env->DeleteLocalRef(methodInfo.classID);
	}
#else
	CCLOG("[WebSocket] wsDisconnect ignored: websocket client unavailable");
#endif
	return 0;
}

static int luaWsIsConnected(lua_State *L)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	lua_pushboolean(L, MacWsIsConnected() ? 1 : 0);
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	JniMethodInfo methodInfo;
	if (!JniHelper::getStaticMethodInfo(methodInfo,
										"re/naruto/game/NarutoSenki",
										"wsIsConnected",
										"()Z"))
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	const jboolean isConnected = methodInfo.env->CallStaticBooleanMethod(methodInfo.classID, methodInfo.methodID);
	methodInfo.env->DeleteLocalRef(methodInfo.classID);
	lua_pushboolean(L, isConnected ? 1 : 0);
#else
	lua_pushboolean(L, 0);
#endif
	return 1;
}

static int luaWsSetMatchConfig(lua_State *L)
{
	const int mapId = (int)lua_tointeger(L, 1);
	const int team = (int)lua_tointeger(L, 2);
	s_networkForcedMapId = mapId;
	s_networkForcedTeam = team;
	const char *yourHero = lua_tostring(L, 3);
	const char *enemyHero = lua_tostring(L, 4);
	const char *yourTeamCsv = lua_tostring(L, 5);
	const char *enemyTeamCsv = lua_tostring(L, 6);
	s_networkYourHeroName = yourHero ? yourHero : "";
	s_networkEnemyHeroName = enemyHero ? enemyHero : "";
	s_networkYourTeamCsv = yourTeamCsv ? yourTeamCsv : "";
	s_networkEnemyTeamCsv = enemyTeamCsv ? enemyTeamCsv : "";
	s_networkOpponentIsBot = lua_gettop(L) >= 7 && lua_toboolean(L, 7);
	return 0;
}

static int luaWsClearMatchConfig(lua_State *L)
{
	s_networkForcedMapId = 0;
	s_networkForcedTeam = -1;
	s_networkYourHeroName.clear();
	s_networkEnemyHeroName.clear();
	s_networkYourTeamCsv.clear();
	s_networkEnemyTeamCsv.clear();
	s_networkOpponentIsBot = false;
	return 0;
}

static void registerLuaNetworkFunctions(lua_State *L)
{
	lua_register(L, "wsConnect", &luaWsConnect);
	lua_register(L, "wsSend", &luaWsSend);
	lua_register(L, "wsDisconnect", &luaWsDisconnect);
	lua_register(L, "wsIsConnected", &luaWsIsConnected);
	lua_register(L, "wsSetMatchConfig", &luaWsSetMatchConfig);
	lua_register(L, "wsClearMatchConfig", &luaWsClearMatchConfig);
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	MacWsSetEventCallback(&onMacWsEvent);
#endif
}
} // namespace

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
extern "C" JNIEXPORT void JNICALL Java_re_naruto_game_NarutoSenki_nativeOnWebSocketEvent(JNIEnv *env,
																						   jclass /*clazz*/,
																						   jstring eventName,
																						   jstring payload)
{
	const char *event = eventName ? env->GetStringUTFChars(eventName, nullptr) : "";
	const char *data = payload ? env->GetStringUTFChars(payload, nullptr) : "";
	const char *safeEvent = event ? event : "";
	const char *safeData = data ? data : "";

	sendLuaEvent(safeEvent, safeData);
	if (s_nativeWsEventCallback)
	{
		s_nativeWsEventCallback(safeEvent, safeData);
	}

	if (eventName && event)
		env->ReleaseStringUTFChars(eventName, event);
	if (payload && data)
		env->ReleaseStringUTFChars(payload, data);
}
#endif

AppDelegate::AppDelegate()
{
}

AppDelegate::~AppDelegate()
{
	SimpleAudioEngine::sharedEngine()->end();
}

bool AppDelegate::applicationDidFinishLaunching()
{
	// 1. initialize lua
	auto pEngine = LuaEngine::defaultEngine();
	CCScriptEngineManager::sharedManager()->setScriptEngine(pEngine);

	auto pStack = pEngine->getLuaStack();
	registerLuaNetworkFunctions(pStack->getLuaState());

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	{
		CFBundleRef bundle = CFBundleGetMainBundle();
		if (bundle)
		{
			CFURLRef url = CFBundleCopyResourcesDirectoryURL(bundle);
			if (url)
			{
				char buf[PATH_MAX];
				if (CFURLGetFileSystemRepresentation(url, true, (UInt8 *)buf, sizeof(buf)))
				{
					std::string root(buf);
					if (!root.empty() && root.back() != '/')
						root.push_back('/');
					// Lua scripts are bundled under Contents/Resources/lua/.
					pEngine->addSearchPath((root + "lua").c_str());
					FileUtils::sharedFileUtils()->addSearchPath(root.c_str());
					FileUtils::sharedFileUtils()->addSearchPath((root + "lua").c_str());
					// Game assets are bundled as a folder reference under
					// Contents/Resources/Resources/, so make that visible to
					// the cocos2d-x file resolver as well.
					FileUtils::sharedFileUtils()->addSearchPath((root + "Resources").c_str());
				}
				CFRelease(url);
			}
		}
	}
#endif
#if (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX || CC_TARGET_PLATFORM == CC_PLATFORM_WIN32 || CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	pEngine->addSearchPath("../lua");
	FileUtils::sharedFileUtils()->addSearchPath("../lua");
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	// pStack->setXXTEAKeyAndSign("2dxLua", strlen("2dxLua"), "XXTEA", strlen("XXTEA"));
	pEngine->addSearchPath("lua");
	FileUtils::sharedFileUtils()->addSearchPath("lua");
	CCLOG("------ Android writable path -> %s", FileUtils::sharedFileUtils()->getWritablePath().c_str());
#endif
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	pEngine->addSearchPath("../../lua");
	FileUtils::sharedFileUtils()->addSearchPath("../../");
	FileUtils::sharedFileUtils()->addSearchPath("../../Resources");
#endif
#ifdef USE_WIN32_CONSOLE
	CCLOG("---------------------------");
	CCLOG("------ DEBUG CONSOLE ------");
	CCLOG("---------------------------\n");

	auto cwd = FileUtils::sharedFileUtils()->getWritablePath();
	string end = "Debug.win32\\";
	// If found use visual studio debug
	// otherwise use visual studio code
	if (cwd.find(end) != string::npos)
	{
		cwd = cwd.substr(0, cwd.length() - end.length());
		auto luaPath = cwd + "projects\\NarutoSenki\\lua";
		auto resPath = cwd + "projects\\NarutoSenki\\Resources";
		auto root = cwd + "projects\\NarutoSenki";
		CCLOG("Lua path: %s", luaPath.c_str());
		CCLOG("Res path: %s", resPath.c_str());
		CCLOG("Root path: %s", root.c_str());
		pEngine->addSearchPath(luaPath.c_str());
		FileUtils::sharedFileUtils()->addSearchPath(luaPath.c_str());
		FileUtils::sharedFileUtils()->addSearchPath(resPath.c_str());
		FileUtils::sharedFileUtils()->addSearchPath(root.c_str());
	}
	CCLOG("Current work path: %s", cwd.c_str());
	CCLOG("---------------------------\n");

#endif

	auto eglView = GLView::sharedOpenGLView();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX) || (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	// get window settings
	pEngine->executeScriptFile(FileUtils::sharedFileUtils()->fullPathForFilename("window.lua").c_str());

	// 2. initialize window
	bool isFullscreen = false;
	int width = 1280;
	int height = 720;
	const char *title = "Naruto Senki";

	lua_getL;
	lua_getglobal(L, "ENABLE_FULL_SCREEN");
	if (lua_isboolean(L, 1))
		isFullscreen = lua_toboolean(L, 1);
	lua_pop(L, 1);
	lua_getglobal(L, "WINDOW_WIDTH");
	if (lua_isnumber(L, 1))
		width = lua_tointeger(L, 1);
	lua_pop(L, 1);
	lua_getglobal(L, "WINDOW_HEIGHT");
	if (lua_isnumber(L, 1))
		height = lua_tointeger(L, 1);
	lua_pop(L, 1);
	lua_getglobal(L, "WINDOW_TITLE");
	if (lua_isstring(L, 1))
		title = lua_tostring(L, 1);
	lua_pop(L, 1);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	eglView->setViewName(title);
	if (!isFullscreen)
	{
		eglView->setFrameSize(width, height);
	}
	else
	{
		// Set the frame size to the full screen value
		eglView->setFrameSize(eglView->getFullscreenWidth(), eglView->getFullscreenHeight());
		eglView->enterFullscreen(0, 0);
	}
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
	//TODO: Support Fullscreen
	eglView->setFrameSize(width, height);
	eglView->setTitle(title);
	eglView->setIcon("icon.png");
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	// On macOS the NSWindow/NSOpenGLView size is created by AppController.
	// Forcing a different logical frame size here (e.g. from window.lua)
	// desynchronizes touch/mouse coordinates from rendered UI.
	// Keep the current view size and only apply the window title.
	eglView->setViewName(title);
#endif
#endif

	// initialize director
	Director *pDirector = Director::sharedDirector();

	pDirector->setOpenGLView(eglView);
	// eglView->setDesignResolutionSize(480, 320, kResolutionFixedHeight);

	// turn on display FPS
	pDirector->setDisplayStats(false);

	// set FPS. the default value is 1.0/60 if you don't call this
	// pDirector->setAnimationInterval(1.0 / 60);

	Internal::initAllSystems();

	// 3. execute main lua script
	string path = FileUtils::sharedFileUtils()->fullPathForFilename("main.lua");
	pEngine->executeScriptFile(path.c_str());

	return true;
}

void AppDelegate::applicationDidEnterBackground()
{
	Director::sharedDirector()->stopAnimation();

	SimpleAudioEngine::sharedEngine()->pauseBackgroundMusic();
}

void AppDelegate::applicationWillEnterForeground()
{
	Director::sharedDirector()->startAnimation();

	SimpleAudioEngine::sharedEngine()->resumeBackgroundMusic();
}
