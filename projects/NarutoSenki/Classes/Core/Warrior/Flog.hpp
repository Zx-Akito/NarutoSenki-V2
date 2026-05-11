#pragma once
#include "CharacterBase.h"
#include "HPBar.h"
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include <cmath>
#endif

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
extern "C"
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
bool MacWsIsConnected();
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
bool NativeBridgeWsIsConnected(void);
#endif
int GetNetworkForcedTeam();
}

static inline bool flogOnlineFollowerMirrorActive()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	return MacWsIsConnected() && GetNetworkForcedTeam() != 0;
#else
	return NativeBridgeWsIsConnected() && GetNetworkForcedTeam() != 0;
#endif
}
#endif

class Flog : public CharacterBase
{
public:
	float _mainPosY = 0;
	float _randomPosX = 0;
	float _randomPosY = 0;
	/** Online lane sync: wave index from spawn order + slot within wave (0..2*kFlogCount-1). */
	uint32_t _netWaveSeq = UINT32_MAX;
	uint8_t _netSlot = 255;
	/** Last authority state applied on follower (Akatsuki) for mirror visuals. */
	State _lastMirrorAuthState = State::IDLE;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	/** Follower client: smooth toward network snap target instead of teleporting every ~100ms (reduces stutter). */
	Vec2 _followNetTargetPos{};
	bool _followNetSmoothActive = false;
#endif

	/** Online follower: facing + walk/attack animations from Konoha snapshots (local AI is off). */
	void syncFollowerMirrorVisual(State authState, bool flipped)
	{
		setFlipX(flipped);
		_isFlipped = flipped;
		_velocity = Vec2(0, 0);
		if (authState == State::DEAD)
			return;
		const bool transient =
			(authState == State::NATTACK || authState == State::HURT || authState == State::AIRHURT ||
			 authState == State::KNOCKDOWN || authState == State::FLOAT);
		const bool stateChanged = (authState != _lastMirrorAuthState);
		if (!stateChanged && !transient)
			return;
		_lastMirrorAuthState = authState;
		stopAllActions();
		switch (authState)
		{
		case State::WALK:
			if (auto *a = getWalkAction())
				runAction(a);
			setState(State::WALK);
			break;
		case State::NATTACK:
			if (auto *a = getNAttackAction())
				runAction(a);
			setState(State::NATTACK);
			break;
		case State::HURT:
			if (auto *a = getHurtAction())
				runAction(a);
			setState(State::HURT);
			break;
		case State::AIRHURT:
			if (auto *a = getAirHurtAction())
				runAction(a);
			setState(State::AIRHURT);
			break;
		case State::KNOCKDOWN:
			if (auto *a = getKnockDownAction())
				runAction(a);
			setState(State::KNOCKDOWN);
			break;
		case State::FLOAT:
			if (auto *a = getFloatAction())
				runAction(a);
			setState(State::FLOAT);
			break;
		default:
			if (auto *a = getIdleAction())
				runAction(a);
			setState(State::IDLE);
			break;
		}
	}
#if COCOS2D_DEBUG
	string _targetName;
#endif

	CREATE_FUNC(Flog);

	bool init()
	{
		RETURN_FALSE_IF(!Sprite::init());

		setAnchorPoint(Vec2(0.5, 0));
		scheduleUpdate();

		return true;
	}

	void setID(const string &name, Role role, Group group)
	{
		clearActionData();
		setName(name);
		setRole(role);
		setGroup(group);

		CCArray *animationArray = CCArray::create();
		auto filePath = format("Unit/Flog/{}.xml", name);
		KTools::readXMLToArray(filePath, animationArray);

		CCArray *tmpAction = (CCArray *)(animationArray->objectAtIndex(0));
		CCArray *tmpData = (CCArray *)(tmpAction->objectAtIndex(0));
		idleArray = (CCArray *)(tmpAction->objectAtIndex(1));

		string unitName;
		uint32_t maxHP;
		int tmpWidth;
		int tmpHeight;
		uint32_t tmpSpeed;
		int tmpCombatPoint;

		readData(tmpData, unitName, maxHP, tmpWidth, tmpHeight, tmpSpeed, tmpCombatPoint);

		setMaxHPValue(maxHP, false);
		setHPValue(maxHP, false);
		setCKR(0);
		setCKR2(0);
		setHeight(tmpHeight);
		setWalkSpeed(tmpSpeed);

		setKillNum(0);

		// init WalkFrame
		tmpAction = (CCArray *)(animationArray->objectAtIndex(1));
		walkArray = (CCArray *)(tmpAction->objectAtIndex(1));

		// init HurtFrame
		tmpAction = (CCArray *)(animationArray->objectAtIndex(2));
		hurtArray = (CCArray *)(tmpAction->objectAtIndex(1));

		// init AirHurtFrame
		tmpAction = (CCArray *)(animationArray->objectAtIndex(3));
		airHurtArray = (CCArray *)(tmpAction->objectAtIndex(1));

		// init KnockDownFrame
		tmpAction = (CCArray *)(animationArray->objectAtIndex(4));
		knockDownArray = (CCArray *)(tmpAction->objectAtIndex(1));

		// init FloatFrame
		tmpAction = (CCArray *)(animationArray->objectAtIndex(5));
		floatArray = (CCArray *)(tmpAction->objectAtIndex(1));

		// init DeadFrame
		tmpAction = (CCArray *)(animationArray->objectAtIndex(6));
		deadArray = (CCArray *)(tmpAction->objectAtIndex(1));

		// init nAttack data & Frame Array
		tmpAction = (CCArray *)(animationArray->objectAtIndex(7));
		tmpData = (CCArray *)(tmpAction->objectAtIndex(0));

		uint32_t tmpValue;
		uint32_t tmpCD;
		readData(tmpData, _nAttackType, tmpValue, _nAttackRangeX, _nAttackRangeY, tmpCD, tmpCombatPoint);
		setNAttackValue(tmpValue);
		nattackArray = (CCArray *)(tmpAction->objectAtIndex(1));

		initAction();
		CCNotificationCenter::sharedNotificationCenter()->addObserver(this, callfuncO_selector(CharacterBase::acceptAttack), "acceptAttack", nullptr);
	}

	void initAction()
	{
		setIdleAction(createAnimation(idleArray, 5, true, false));
		setWalkAction(createAnimation(walkArray, 10, true, false));
		setHurtAction(createAnimation(hurtArray, 10, false, true));

		setAirHurtAction(createAnimation(airHurtArray, 10, false, false));
		setKnockDownAction(createAnimation(knockDownArray, 10, false, true));
		setDeadAction(createAnimation(deadArray, 10, false, false));
		setFloatAction(createAnimation(floatArray, 10, false, false));

		setNAttackAction(createAnimation(nattackArray, 10, false, true));
	}

	void setHPbar()
	{
		if (getGroup() != getGameLayer()->currentPlayer->getGroup())
			_hpBar = HPBar::create("flog_bar_r.png");
		else
			_hpBar = HPBar::create("flog_bar.png");
		_hpBar->setPositionY(getHeight());
		_hpBar->setDelegate(this);
		addChild(_hpBar);
	}

	void update(float dt) override
	{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
		if (flogOnlineFollowerMirrorActive() && _followNetSmoothActive &&
			getState() != State::DEAD)
		{
			const Vec2 cur = getPosition();
			const Vec2 &tgt = _followNetTargetPos;
			const float dx = tgt.x - cur.x;
			const float dy = tgt.y - cur.y;
			const float dsq = dx * dx + dy * dy;
			if (dsq > 360000.f)
			{
				setPosition(tgt);
			}
			else if (dsq > 3.f)
			{
				const float dist = sqrtf(dsq);
				const float maxStep = 920.f * dt;
				const float step = dist > maxStep ? maxStep : dist;
				const float inv = 1.f / dist;
				setPosition(Vec2(cur.x + dx * inv * step, cur.y + dy * inv * step));
			}
			else
				setPosition(tgt);
			if (auto *par = getParent())
				par->reorderChild(this, -(int)getPositionY());
		}
#endif
		CharacterBase::update(dt);
	}

protected:
	void dealloc()
	{
		setState(State::DEAD);
		unscheduleAllSelectors();
		stopAllActions();

		if (isKonohaGroup())
			std::erase(getGameLayer()->_KonohaFlogArray, this);
		else
			std::erase(getGameLayer()->_AkatsukiFlogArray, this);

		removeFromParent();
	}

	void setAI(float dt)
	{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
		if (flogOnlineFollowerMirrorActive())
			return;
#endif
		if (isFreeState())
		{
			if (!_randomPosY)
			{
				_randomPosY = rand() % 8 + 4;
				_randomPosX = rand() % 32 + 20;
			}

			bool hasTarget = false;

			if (_mainTarget)
			{
				if (_mainTarget->getState() != State::DEAD && !_mainTarget->_isInvincible && _mainTarget->_isVisable &&
					_mainTarget->getGroup() != getGroup())
				{
					hasTarget = true;
				}
			}

			if (!hasTarget)
			{
				if (notFindFlog(0))
				{
					if (notFindHero(64))
					{
						if (notFindTower(0))
						{
							_mainTarget = nullptr;
						}
					}
				}
			}
#if COCOS2D_DEBUG
			if (_mainTarget)
				_targetName = _mainTarget->getName();
#endif

			Vec2 moveDirection;

			if (_mainTarget)
			{
				Vec2 sp = getDistanceToTargetAndIgnoreOriginY();
				if (abs(sp.x) > _randomPosX || abs(sp.y) > _randomPosY)
				{
					if (abs(sp.x) > 64 && _mainTarget->isNotFlog() && _mainTarget->isNotTower())
					{
						_mainTarget = nullptr;
						return;
					}

					moveDirection = sp.getNormalized();
					walk(moveDirection);
				}
				else
				{
					if (_state != State::NATTACK)
					{
						changeSide(sp);
						attack(NAttack);
					}
				}
				return;
			}

			// no target then step on
			if (abs(getPositionY() - _mainPosY) > 8)
			{
				if (isKonohaGroup())
					moveDirection = Vec2(1, getPositionY() > _mainPosY ? -1 : 1).getNormalized();
				else
					moveDirection = Vec2(-1, getPositionY() > _mainPosY ? -1 : 1).getNormalized();
			}
			else
			{
				if (isKonohaGroup())
					moveDirection = Vec2(1, 0).getNormalized();
				else
					moveDirection = Vec2(-1, 0).getNormalized();
			}
			walk(moveDirection);
		}
	}
};
