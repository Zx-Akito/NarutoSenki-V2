#include "GameLayer.h"

// GameLayerInputControl.inl
// - Keeps local control wiring in one place (joystick/buttons/keyboard hooks).
// - Calls network-input helpers only for player input replication events.
// - Does not own gameplay simulation or scene flow.

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
extern "C" void MacKeyboard_register();
extern "C" void MacKeyboard_unregister();
#endif

void GameLayer::setKeyEventHandler()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	Director::sharedDirector()->getOpenGLView()->setAccelerometerKeyHook((GLView::LPFN_ACCELEROMETER_KEYHOOK)(&GameLayer::LPFN_ACCELEROMETER_KEYHOOK));
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
	glfwSetKeyCallback(_window, keyEventHandle);
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	MacKeyboard_register();
#endif
}

void GameLayer::removeKeyEventHandler()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	Director::sharedDirector()->getOpenGLView()->setAccelerometerKeyHook(nullptr);
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
	glfwSetKeyCallback(_window, nullptr);
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	MacKeyboard_unregister();
#endif
}

void GameLayer::JoyStickRelease()
{
	if (currentPlayer->getState() == State::WALK)
	{
		currentPlayer->idle();
	}
	sendNetworkJoyReleaseEvent();
}

void GameLayer::JoyStickUpdate(Vec2 direction)
{
	if (!ougisChar)
	{
		if (shouldBlockNetworkBattleInputEcho())
			return;
		currentPlayer->walk(direction);
		// Online: walk() is a no-op for the local player during NATTACK and other blocked states,
		// but we must not still send joy_update — the peer would apply it to the enemy hero mirror.
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
		if (MacWsIsConnected() && currentPlayer->getState() == State::WALK)
			sendNetworkJoyUpdateEvent(direction.x, direction.y);
#else
		sendNetworkJoyUpdateEvent(direction.x, direction.y);
#endif
	}
}

void GameLayer::attackButtonClick(ABType type)
{
	if (shouldBlockNetworkBattleInputEcho())
		return;
	if (type == NAttack)
	{
		_isAttackButtonRelease = false;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
		if (MacWsIsConnected())
			sendNetworkJoyReleaseEvent();
#endif
	}

	if (type == Item1)
	{
		currentPlayer->setItem(type);
	}
	else
	{
		currentPlayer->attack(type);
	}
	sendNetworkInputEvent("attack_click",
						 format("{{\"type\":{}}}", (int)type));
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	if (MacWsIsConnected() && type != Item1)
	{
		sendNetworkOwnedHeroPositionSnapIfNeeded(currentPlayer->getPosition(),
													HeroSnapKind::ImmediateBurst);
	}
#endif
}

void GameLayer::gearButtonClick(GearType type)
{
	if (shouldBlockNetworkBattleInputEcho())
		return;
	currentPlayer->useGear(type);
	sendNetworkInputEvent("gear_click",
						 format("{{\"type\":{}}}", (int)type));
}

void GameLayer::attackButtonRelease()
{
	if (shouldBlockNetworkBattleInputEcho())
		return;
	_isAttackButtonRelease = true;
	sendNetworkInputEvent("attack_release");
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32 || CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#define isPressed(__KEY__) _isPressed(__KEY__)
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
#define isPressed(__KEY__) glfwGetKey(_window, __KEY__)
#endif

/**
 * Use __W __S __D __A control when to force move
 */
#define MOVE(__W, __S, __D, __A, name, keyState)                                    \
	{                                                                               \
		if (keyState)                                                               \
			_gLayer->_lastPressedMovementKey = name;                                \
		else if (_gLayer->_lastPressedMovementKey == name)                          \
			_gLayer->_lastPressedMovementKey = -100;                                \
		int horizontal;                                                             \
		int vertical;                                                               \
		if (__W)                                                                    \
		{                                                                           \
			vertical = (isPressed(KEY_W) ? 1 : -1);                                 \
		}                                                                           \
		else if (__S)                                                               \
		{                                                                           \
			vertical = (isPressed(KEY_S) ? -1 : 1);                                 \
		}                                                                           \
		else                                                                        \
		{                                                                           \
			vertical = (isPressed(KEY_W) ? 1 : -1) + (isPressed(KEY_S) ? -1 : 1);   \
			vertical = abs(vertical) > 1 ? vertical / 2 : vertical;                 \
		}                                                                           \
		if (__D)                                                                    \
		{                                                                           \
			horizontal = (isPressed(KEY_D) ? 1 : -1);                               \
		}                                                                           \
		else if (__A)                                                               \
		{                                                                           \
			horizontal = (isPressed(KEY_A) ? -1 : 1);                               \
		}                                                                           \
		else                                                                        \
		{                                                                           \
			horizontal = (isPressed(KEY_D) ? 1 : -1) + (isPressed(KEY_A) ? -1 : 1); \
			horizontal = abs(horizontal) > 1 ? horizontal / 2 : horizontal;         \
		}                                                                           \
		if (horizontal != 0 || vertical != 0)                                       \
		{                                                                           \
			if (!_gLayer->ougisChar && !_gLayer->shouldBlockNetworkBattleInputEcho()) \
				_gLayer->currentPlayer->walk(Vec2(horizontal, vertical));           \
		}                                                                           \
		else if (_gLayer->currentPlayer->getState() == State::WALK)                 \
		{                                                                           \
			_gLayer->_lastPressedMovementKey = -100;                                \
			_gLayer->currentPlayer->idle();                                         \
		}                                                                           \
		break;                                                                      \
	}

void GameLayer::keyEventHandle(GLFWwindow *window, int key, int scancode, int keyState, int mods)
{
	if (!_gLayer || !_gLayer->currentPlayer)
		return;

	switch (key)
	{
	case KEY_W:
		MOVE(keyState, 0, 0, 0, KEY_W, keyState);
	case KEY_S:
		MOVE(0, keyState, 0, 0, KEY_S, keyState);
	case KEY_A:
		MOVE(0, 0, keyState, 0, KEY_A, keyState);
	case KEY_D:
		MOVE(0, 0, 0, keyState, KEY_D, keyState);
	case KEY_J:
		if (keyState)
			_gLayer->_hudLayer->nAttackButton->click();
		else
			_gLayer->_isAttackButtonRelease = true;
		break;
	case KEY_L:
		if (keyState)
			_gLayer->_hudLayer->item1Button->click();
		break;
	case KEY_H:
		if (keyState)
			_gLayer->_hudLayer->skill5Button->click();
		break;
	case KEY_K:
		if (keyState)
			_gLayer->_hudLayer->skill4Button->click();
		break;
	case KEY_U: // skill 1
		if (keyState)
			_gLayer->_hudLayer->skill1Button->click();
		break;
	case KEY_I: // skill 2
		if (keyState)
			_gLayer->_hudLayer->skill2Button->click();
		break;
	case KEY_O: // skill 3
		if (keyState)
			_gLayer->_hudLayer->skill3Button->click();
		break;
	case KEY_B:
		if (keyState)
		{
			if (_gLayer->_isGear)
			{
				_gLayer->_gearLayer->confirmPurchase();
			}
			else
			{
				_gLayer->_hudLayer->getItem3Button()->click();
			}
		}
		break;
	case KEY_N:
		if (keyState)
			_gLayer->_hudLayer->getItem4Button()->click();
		break;
	case KEY_M:
		if (keyState)
			_gLayer->_hudLayer->getItem2Button()->click();
		break;
	case KEY_SPACE:
		if (_gLayer->_enableGear && _gLayer->_isStarted && keyState && !_gLayer->_isPause)
		{
			if (_gLayer->_isGear)
				_gLayer->dismissGearOverlay();
			else
			{
				_gLayer->onGear();
			}
		}
		break;
	case KEY_ESCAPE:
	case KEY_ENTER:
		if (keyState && _gLayer->_isStarted)
		{
			if (_gLayer->_isPause)
			{
				_gLayer->resumeFromPause();
			}
			else if (_gLayer->_isGear)
				_gLayer->dismissGearOverlay();
			else
			{
				_gLayer->onPause();
			}
		}
		break;
	case KEY_F11:
		break;
	}
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
void GameLayer::LPFN_ACCELEROMETER_KEYHOOK(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		keyEventHandle(nullptr, wParam, 0, 1, 0);
		break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		keyEventHandle(nullptr, wParam, 0, 0, 0);
		break;
	}
}
#endif

bool GameLayer::checkHasAnyMovement()
{
	if (_gLayer && _gLayer->_lastPressedMovementKey != -100)
	{
		keyEventHandle(_window, _gLayer->_lastPressedMovementKey, 0, 1, 0);
		return true;
	}
	return false;
}

#elif (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)

static bool s_macKeyState[256] = {};

#define isPressed(__KEY__) ((__KEY__) < 256 && s_macKeyState[__KEY__])

#define MOVE_MAC(__W, __S, __D, __A, name, keyState)                                    \
	{                                                                               \
		if (keyState)                                                               \
			_gLayer->_lastPressedMovementKey = name;                                \
		else if (_gLayer->_lastPressedMovementKey == name)                          \
			_gLayer->_lastPressedMovementKey = -100;                                \
		int horizontal;                                                             \
		int vertical;                                                               \
		if (__W)                                                                    \
		{                                                                           \
			vertical = (isPressed(KEY_W) ? 1 : -1);                                \
		}                                                                           \
		else if (__S)                                                               \
		{                                                                           \
			vertical = (isPressed(KEY_S) ? -1 : 1);                                \
		}                                                                           \
		else                                                                        \
		{                                                                           \
			vertical = (isPressed(KEY_W) ? 1 : -1) + (isPressed(KEY_S) ? -1 : 1);  \
			vertical = abs(vertical) > 1 ? vertical / 2 : vertical;                \
		}                                                                           \
		if (__D)                                                                    \
		{                                                                           \
			horizontal = (isPressed(KEY_D) ? 1 : -1);                              \
		}                                                                           \
		else if (__A)                                                               \
		{                                                                           \
			horizontal = (isPressed(KEY_A) ? -1 : 1);                              \
		}                                                                           \
		else                                                                        \
		{                                                                           \
			horizontal = (isPressed(KEY_D) ? 1 : -1) + (isPressed(KEY_A) ? -1 : 1); \
			horizontal = abs(horizontal) > 1 ? horizontal / 2 : horizontal;        \
		}                                                                           \
		if (horizontal != 0 || vertical != 0)                                       \
		{                                                                           \
			if (!_gLayer->ougisChar && !_gLayer->shouldBlockNetworkBattleInputEcho()) \
			{                                                                       \
				auto *cp = _gLayer->currentPlayer;                                  \
				cp->walk(Vec2(horizontal, vertical));                             \
				if (MacWsIsConnected() && cp->getState() == State::WALK)          \
					sendNetworkJoyUpdateEvent((float)horizontal, (float)vertical); \
			}                                                                       \
		}                                                                           \
		else if (_gLayer->currentPlayer->getState() == State::WALK)                 \
		{                                                                           \
			_gLayer->_lastPressedMovementKey = -100;                                \
			_gLayer->currentPlayer->idle();                                         \
			sendNetworkJoyReleaseEvent();                                           \
		}                                                                           \
		break;                                                                      \
	}

void GameLayer::keyEventHandle(int key, int keyState)
{
	if (!_gLayer || !_gLayer->currentPlayer)
		return;

	if (key >= 0 && key < 256)
		s_macKeyState[key] = (keyState != 0);

	switch (key)
	{
	case KEY_W: MOVE_MAC(keyState, 0, 0, 0, KEY_W, keyState);
	case KEY_S: MOVE_MAC(0, keyState, 0, 0, KEY_S, keyState);
	case KEY_A: MOVE_MAC(0, 0, keyState, 0, KEY_A, keyState);
	case KEY_D: MOVE_MAC(0, 0, 0, keyState, KEY_D, keyState);
	case KEY_J:
		if (!_gLayer->shouldBlockNetworkBattleInputEcho())
		{
			if (keyState) _gLayer->_hudLayer->nAttackButton->click();
			else { _gLayer->_isAttackButtonRelease = true; sendNetworkInputEvent("attack_release"); }
		}
		break;
	case KEY_L: if (keyState) _gLayer->_hudLayer->item1Button->click(); break;
	case KEY_H: if (keyState) _gLayer->_hudLayer->skill5Button->click(); break;
	case KEY_K: if (keyState) _gLayer->_hudLayer->skill4Button->click(); break;
	case KEY_U: if (keyState) _gLayer->_hudLayer->skill1Button->click(); break;
	case KEY_I: if (keyState) _gLayer->_hudLayer->skill2Button->click(); break;
	case KEY_O: if (keyState) _gLayer->_hudLayer->skill3Button->click(); break;
	case KEY_1: case KEY_KP_1:
		if (_gLayer->_isGear && keyState) { auto &gb = _gLayer->_gearLayer->_screwLayer->getGearBtnArray(); if (gb.size()>=1 && gb.at(0)) gb.at(0)->click(); } break;
	case KEY_2: case KEY_KP_2:
		if (_gLayer->_isGear && keyState) { auto &gb = _gLayer->_gearLayer->_screwLayer->getGearBtnArray(); if (gb.size()>=2 && gb.at(1)) gb.at(1)->click(); } break;
	case KEY_3: case KEY_KP_3:
		if (_gLayer->_isGear && keyState) { auto &gb = _gLayer->_gearLayer->_screwLayer->getGearBtnArray(); if (gb.size()>=3 && gb.at(2)) gb.at(2)->click(); } break;
	case KEY_B:
		if (keyState) { if (_gLayer->_isGear) _gLayer->_gearLayer->confirmPurchase(); else _gLayer->_hudLayer->getItem3Button()->click(); } break;
	case KEY_N:
		if (keyState) _gLayer->_hudLayer->getItem4Button()->click(); break;
	case KEY_M:
		if (keyState) _gLayer->_hudLayer->getItem2Button()->click(); break;
	case KEY_SPACE:
		if (_gLayer->_enableGear && _gLayer->_isStarted && keyState && !_gLayer->_isPause)
		{
			if (_gLayer->_isGear) _gLayer->dismissGearOverlay();
			else _gLayer->onGear();
		}
		break;
	case KEY_ESCAPE: case KEY_ENTER:
		if (keyState && _gLayer->_isStarted)
		{
			if (_gLayer->_isPause) { _gLayer->resumeFromPause(); }
			else if (_gLayer->_isGear) _gLayer->dismissGearOverlay();
			else { _gLayer->onPause(); }
		}
		break;
	}
}

bool GameLayer::checkHasAnyMovement()
{
	if (_gLayer && _gLayer->_lastPressedMovementKey != -100)
	{
		keyEventHandle(_gLayer->_lastPressedMovementKey, 1);
		return true;
	}
	return false;
}

#else
bool GameLayer::checkHasAnyMovement()
{
	return false;
}
#endif
