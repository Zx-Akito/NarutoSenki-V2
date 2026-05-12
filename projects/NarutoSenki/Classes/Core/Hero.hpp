#pragma once
#include "Element.h"

typedef std::function<void()> AIHandler;

class Hero : public CharacterBase
{
	friend class CommandSystem;

private:
	Sprite *rebornSprite = nullptr;
	CCLabelBMFont *rebornLabel = nullptr;
	// Hero *proxy = nullptr;

public:
	Hero()
	{
	}

	~Hero()
	{
		CC_SAFE_RELEASE(idleArray);
		CC_SAFE_RELEASE(walkArray);
		CC_SAFE_RELEASE(deadArray);
		CC_SAFE_RELEASE(hurtArray);
		CC_SAFE_RELEASE(airHurtArray);
		CC_SAFE_RELEASE(floatArray);
		CC_SAFE_RELEASE(knockDownArray);
		CC_SAFE_RELEASE(nattackArray);
		CC_SAFE_RELEASE(skillSPC1Array);
		CC_SAFE_RELEASE(skillSPC2Array);
		CC_SAFE_RELEASE(skillSPC3Array);
		CC_SAFE_RELEASE(skillSPC4Array);
		CC_SAFE_RELEASE(skillSPC5Array);
		CC_SAFE_RELEASE(skill1Array);
		CC_SAFE_RELEASE(skill2Array);
		CC_SAFE_RELEASE(skill3Array);
		CC_SAFE_RELEASE(skill4Array);
		CC_SAFE_RELEASE(skill5Array);
	}

	bool init()
	{
		RETURN_FALSE_IF(!Sprite::init());

		setAnchorPoint(Vec2(0.5, 0));
		scheduleUpdate();

		return true;
	}

	virtual void setID(const string &name, Role role, Group group)
	{
		clearActionData();
		setRole(role);
		setGroup(group);

		auto fileUtils = FileUtils::sharedFileUtils();
		string fpath = fileUtils->fullPathForFilename(format("Unit/Ninja/{}/{}.xml", name, name).c_str());
		if (!fileUtils->isFileExist(fpath))
		{
			fpath = fileUtils->fullPathForFilename(format("Unit/Kuchiyose/{}/{}.xml", name, name).c_str());
			if (!fileUtils->isFileExist(fpath))
			{
				fpath = fileUtils->fullPathForFilename(format("Unit/Kugutsu/{}/{}.xml", name, name).c_str());
				if (!fileUtils->isFileExist(fpath))
				{
					fpath = fileUtils->fullPathForFilename(format("Unit/Guardian/{}/{}.xml", name, name).c_str());
					if (!fileUtils->isFileExist(fpath))
					{
						CCLOGERROR("Not found file %s", fpath.c_str());
						return;
					}
				}
			}
		}
		CCArray *animationArray = CCArray::create();
		KTools::readXMLToArray(fpath, animationArray);

		// init
		CCArray *tmpAction = (CCArray *)(animationArray->objectAtIndex(0));
		CCArray *tmpData = (CCArray *)(tmpAction->objectAtIndex(0));
		idleArray = (CCArray *)(tmpAction->objectAtIndex(1));
		idleArray->retain();

		string unitName;
		uint32_t maxHP;
		int tmpWidth;
		int tmpHeight;
		uint32_t tmpSpeed;
		int tmpCombatPoint;

		readData(tmpData, unitName, maxHP, tmpWidth, tmpHeight, tmpSpeed, tmpCombatPoint);

		if (getName().empty()) // Set hp when character is not awaken
		{
			setMaxHPValue(maxHP, false);
			setHPValue(maxHP, false);
		}
		setCKR(0);
		setCKR2(0);
		setHeight(tmpHeight);
		setWalkSpeed(tmpSpeed);
		_originSpeed = tmpSpeed;

		setKillNum(0);

		// init WalkFrame
		tmpAction = (CCArray *)(animationArray->objectAtIndex(1));
		walkArray = (CCArray *)(tmpAction->objectAtIndex(1));
		walkArray->retain();

		// init HurtFrame
		tmpAction = (CCArray *)(animationArray->objectAtIndex(2));
		hurtArray = (CCArray *)(tmpAction->objectAtIndex(1));
		hurtArray->retain();

		// init AirHurtFrame
		tmpAction = (CCArray *)(animationArray->objectAtIndex(3));
		airHurtArray = (CCArray *)(tmpAction->objectAtIndex(1));
		airHurtArray->retain();

		// init KnockDownFrame
		tmpAction = (CCArray *)(animationArray->objectAtIndex(4));
		knockDownArray = (CCArray *)(tmpAction->objectAtIndex(1));
		knockDownArray->retain();

		// init FloatFrame
		tmpAction = (CCArray *)(animationArray->objectAtIndex(5));
		floatArray = (CCArray *)(tmpAction->objectAtIndex(1));
		floatArray->retain();

		// init DeadFrame
		tmpAction = (CCArray *)(animationArray->objectAtIndex(6));
		deadArray = (CCArray *)(tmpAction->objectAtIndex(1));
		deadArray->retain();

		// init nAttack data & Frame Array
		tmpAction = (CCArray *)(animationArray->objectAtIndex(7));
		tmpData = (CCArray *)(tmpAction->objectAtIndex(0));

		uint32_t tmpValue;
		uint32_t tmpCD;
		readData(tmpData, _nAttackType, tmpValue, _nAttackRangeX, _nAttackRangeY, tmpCD, tmpCombatPoint);
		setNAttackValue(tmpValue);

		nattackArray = (CCArray *)(tmpAction->objectAtIndex(1));
		nattackArray->retain();

		// init skill1 data & Frame Array
		tmpAction = (CCArray *)(animationArray->objectAtIndex(8));
		tmpData = (CCArray *)(tmpAction->objectAtIndex(0));

		readData(tmpData, _sAttackType1, tmpValue, _sAttackRangeX1, _sAttackRangeY1, _sAttackCD1, _sAttackCombatPoint1);
		setSAttackValue1(tmpValue);

		skill1Array = (CCArray *)(tmpAction->objectAtIndex(1));
		skill1Array->retain();

		// init skill2 data & Frame Array
		tmpAction = (CCArray *)(animationArray->objectAtIndex(9));
		tmpData = (CCArray *)(tmpAction->objectAtIndex(0));
		readData(tmpData, _sAttackType2, tmpValue, _sAttackRangeX2, _sAttackRangeY2, _sAttackCD2, _sAttackCombatPoint2);
		setSAttackValue2(tmpValue);
		skill2Array = (CCArray *)(tmpAction->objectAtIndex(1));
		skill2Array->retain();

		if (name == "Kakashi")
		{
		}
		else if (name == "Minato")
		{
			_sAttack1isDouble = true;
		}

		// init skill3 data & Frame Array
		tmpAction = (CCArray *)(animationArray->objectAtIndex(10));
		tmpData = (CCArray *)(tmpAction->objectAtIndex(0));
		readData(tmpData, _sAttackType3, tmpValue, _sAttackRangeX3, _sAttackRangeY3, _sAttackCD3, _sAttackCombatPoint3);
		setSAttackValue3(tmpValue);
		skill3Array = (CCArray *)(tmpAction->objectAtIndex(1));
		skill3Array->retain();

		// init skill4 data & Frame Array
		tmpAction = (CCArray *)(animationArray->objectAtIndex(11));
		tmpData = (CCArray *)(tmpAction->objectAtIndex(0));
		readData(tmpData, _sAttackType4, tmpValue, _sAttackRangeX4, _sAttackRangeY4, _sAttackCD4, _sAttackCombatPoint4);
		setSAttackValue4(tmpValue);
		skill4Array = (CCArray *)(tmpAction->objectAtIndex(1));
		skill4Array->retain();

		// init skill5 data & Frame Array
		tmpAction = (CCArray *)(animationArray->objectAtIndex(12));
		tmpData = (CCArray *)(tmpAction->objectAtIndex(0));
		readData(tmpData, _sAttackType5, tmpValue, _sAttackRangeX5, _sAttackRangeY5, _sAttackCD5, _sAttackCombatPoint5);
		setSAttackValue5(tmpValue);
		skill5Array = (CCArray *)(tmpAction->objectAtIndex(1));
		skill5Array->retain();

		// initial specal animations
		while (1)
		{
			CC_BREAK_IF(animationArray->count() <= 15);
			tmpAction = (CCArray *)(animationArray->objectAtIndex(15));
			skillSPC5Array = (CCArray *)(tmpAction->objectAtIndex(1));
			skillSPC5Array->retain();

			CC_BREAK_IF(animationArray->count() <= 16);
			tmpAction = (CCArray *)(animationArray->objectAtIndex(16));
			skillSPC4Array = (CCArray *)(tmpAction->objectAtIndex(1));
			skillSPC4Array->retain();

			CC_BREAK_IF(animationArray->count() <= 17);
			tmpAction = (CCArray *)(animationArray->objectAtIndex(17));
			tmpData = (CCArray *)(tmpAction->objectAtIndex(0));
			readData(tmpData, _spcAttackType1, tmpValue, _spcAttackRangeX1, _spcAttackRangeY1, _spcAttackCD1, tmpCombatPoint);
			if (!_spcAttackType1.empty())
			{
				skillSPC1Array = (CCArray *)(tmpAction->objectAtIndex(1));
				setSpcAttackValue1(tmpValue);
				skillSPC1Array->retain();
			}

			CC_BREAK_IF(animationArray->count() <= 18);
			tmpAction = (CCArray *)(animationArray->objectAtIndex(18));
			tmpData = (CCArray *)(tmpAction->objectAtIndex(0));
			readData(tmpData, _spcAttackType2, tmpValue, _spcAttackRangeX2, _spcAttackRangeY2, _spcAttackCD2, tmpCombatPoint);
			if (!_spcAttackType2.empty())
			{
				skillSPC2Array = (CCArray *)(tmpAction->objectAtIndex(1));
				setSpcAttackValue2(tmpValue);
				skillSPC2Array->retain();
			}

			CC_BREAK_IF(animationArray->count() <= 19);
			tmpAction = (CCArray *)(animationArray->objectAtIndex(19));
			tmpData = (CCArray *)(tmpAction->objectAtIndex(0));
			readData(tmpData, _spcAttackType3, tmpValue, _spcAttackRangeX3, _spcAttackRangeY3, _spcAttackCD3, tmpCombatPoint);
			if (!_spcAttackType3.empty())
			{
				skillSPC3Array = (CCArray *)(tmpAction->objectAtIndex(1));
				setSpcAttackValue3(tmpValue);
				skillSPC3Array->retain();
			}
			break;
		}

		_isArmored = false;
		if (name == HeroEnum::Kiba)
		{
			_isArmored = true;
		}
		else if (name == HeroEnum::Kakuzu)
		{
			_heartEffect = Sprite::createWithSpriteFrameName("Heart_Effect_00");
			_heartEffect->setPosition(Vec2(getContentSize().width + 40, 70));
			addChild(_heartEffect);
		}

		// Heroes have 50 coins by default
		if (getLV() == 1 && getCoin() == 0) // TODO: Move to game layer
		{
			setCoin(50);
		}

		if (!getName().empty() && isPlayer())
		{
			auto oldCharName = getName();
			bool isUpdateUI = oldCharName != name;
			setName(name);

			if (isUpdateUI)
				getGameLayer()->updateHudSkillButtons();
		}
		else
		{
			setName(name);
		}

		initAction();
		CCNotificationCenter::sharedNotificationCenter()->addObserver(this, callfuncO_selector(CharacterBase::acceptAttack), "acceptAttack", nullptr);
	}

	void initAction()
	{
		setIdleAction(createAnimation(idleArray, 5, true, false));
		setWalkAction(createAnimation(walkArray, 10, true, false));
		setHurtAction(createAnimation(hurtArray, 10, false, false));

		setAirHurtAction(createAnimation(airHurtArray, 10, false, false));
		setKnockDownAction(createAnimation(knockDownArray, 10, false, true));
		setDeadAction(createAnimation(deadArray, 10, false, false));
		setFloatAction(createAnimation(floatArray, 10, false, false));

		setNAttackAction(createAnimation(nattackArray, 10, false, true));
		setSkill1Action(createAnimation(skill1Array, 10, false, true));
		setSkill2Action(createAnimation(skill2Array, 10, false, true));
		setSkill3Action(createAnimation(skill3Array, 10, false, true));
		setSkill4Action(createAnimation(skill4Array, 10, false, true));
		setSkill5Action(createAnimation(skill5Array, 10, false, true));
	}

	void setShadows()
	{
		if (!_shadow)
		{
			_shadow = Sprite::createWithSpriteFrameName("shadows");
			_shadow->setAnchorPoint(Vec2(0.5, 0.5));
			_shadow->setPosition(getPosition());
			getGameLayer()->shadowBatch->addChild(_shadow);
		}
	}

	void setHPbar()
	{
		if (getGameLayer()->playerGroup != getGroup())
		{
			_hpBar = HPBar::create("hp_bar_r.png");
		}
		else if (isCom() || isClone() || isKugutsu() || isSummon())
		{
			_hpBar = HPBar::create("hp_bar_b.png");
		}
		else if (isPlayer())
		{
			_hpBar = HPBar::create("hp_bar.png");
		}
		_hpBar->setPositionY(getHeight());
		_hpBar->setDelegate(this);
		addChild(_hpBar);
		tryCreateNetworkBattleNicknameLabel();
		changeHPbar();
	}

	virtual void changeHPbar()
	{
		if (_exp >= 500 && _level == 1)
		{
			_level = 2;
			setCKR(getCKR() + 15000);
			_isCanOugis1 = true;
			if (isPlayer())
			{
				getGameLayer()->setCKRLose(false);
				getGameLayer()->removeOugisMark(1);
			}
			setMaxHPValue(getMaxHP() + 500);
			setNAttackValue(getNAttackValue() + 9);
			_rebornTime += 1;
		}
		else if (_exp >= 1000 && _level == 2)
		{
			_level = 3;
			setMaxHPValue(getMaxHP() + 1000);
			setNAttackValue(getNAttackValue() + 18);
			_rebornTime += 2;
		}
		else if (_exp >= 1500 && _level == 3)
		{
			_level = 4;
			setCKR2(getCKR2() + 25000);
			_isCanOugis2 = true;
			if (isPlayer())
			{
				getGameLayer()->setCKRLose(true);
				getGameLayer()->removeOugisMark(2);
			}
			setMaxHPValue(getMaxHP() + 2000);
			setNAttackValue(getNAttackValue() + 27);
			_rebornTime += 3;
		}
		else if (_exp >= 2000 && _level == 4)
		{
			_level = 5;
			setMaxHPValue(getMaxHP() + 2500);
			setNAttackValue(getNAttackValue() + 36);
			_rebornTime += 4;
		}
		else if (_exp >= 2500 && _level == 5)
		{
			_level = 6;
			setMaxHPValue(getMaxHP() + 3000);
			setNAttackValue(getNAttackValue() + 45);
			_rebornTime += 5;
		}

		if (_hpBar)
		{
			auto frame = getSpriteFrame("hp_bottom{}.png", _level);
			_hpBar->getHPBottom()->setDisplayFrame(frame);
		}
	}

	virtual void reborn(float dt)
	{
		if (!enableReborn)
		{
			// TODO: Spectating Mode
			// if (isPlayer())
			// getGameLayer()->getHudLayer()->enableSpectatingMode();
			return;
		}

		CharacterBase::reborn(dt);
		// If the character has changed, then cleanup and return
		if (changeCharId > -1)
		{
			if (rebornSprite)
			{
				rebornSprite->removeFromParent();
				rebornSprite = nullptr;
			}
			checkRefCount(0);
			return;
		}

		setPosition(getSpawnPoint());

		if (getPosition().equals(getSpawnPoint()))
		{
			CCNotificationCenter::sharedNotificationCenter()->addObserver(this, callfuncO_selector(CharacterBase::acceptAttack), "acceptAttack", nullptr);
			setOpacity(255);
			setHPValue(getMaxHP(), false);
			setHPbar();
			_state = State::HURT;

			if (getLV() < 4)
			{
				if (isKonohaGroup())
					setEXP(getEXP() + getGameLayer()->kEXPBound);
				else
					setEXP(getEXP() + getGameLayer()->aEXPBound);

				changeHPbar();

				if (isPlayer())
				{
					getGameLayer()->getHudLayer()->setEXPLose();
				}
			}

			if (isKonohaGroup())
			{
				if (_isFlipped)
				{
					setFlipX(false);
					_isFlipped = false;
				}
			}
			else
			{
				if (!_isFlipped)
				{
					setFlipX(true);
					_isFlipped = true;
				}
			}
			idle();
			if (rebornSprite)
			{
				unschedule(schedule_selector(Hero::countDown));
				rebornSprite->removeFromParent();
				rebornSprite = nullptr;
			}
			if (isNotPlayer())
			{
				doAI();
			}
			else
			{
				if (_isAI)
				{
					doAI();
				}
				getGameLayer()->getHudLayer()->status_hpbar->setOpacity(255);
				getGameLayer()->setHPLose(getHpPercent());
			}
			scheduleUpdate();
		}
		getGameLayer()->reorderChild(this, -getPositionY());
	}

	virtual void dealloc()
	{
		CCNotificationCenter::sharedNotificationCenter()->removeAllObservers(this);
		stopAllActions();
		_state = State::DEAD;

		if (!_monsterArray.empty() && getName() != HeroEnum::Minato)
		{
			for (auto mo : _monsterArray)
			{
				if (auto heroMo = dynamic_cast<Hero *>(mo))
				{
					heroMo->dealloc();
				}
				else
				{
					CCNotificationCenter::sharedNotificationCenter()->removeAllObservers(mo);
					mo->stopAllActions();
					mo->unscheduleAllSelectors();
					mo->setState(State::DEAD);
					mo->removeFromParent();
				}
			}
			_monsterArray.clear();
		}

		if (isClone() || isKugutsu() || isSummon())
		{
			unschedule(schedule_selector(CharacterBase::setAI));

			std::erase(getGameLayer()->_CharacterArray, this);
			getGameLayer()->clearAllFlogsMainTarget(this);

			if (_shadow)
				_shadow->removeFromParent();

			if (_master)
				_master->removeMon(this);

			removeFromParent();
		}
		else if (isNotGuardian())
		{
			if (rebornLabelTime == 3)
			{
				scheduleOnce(schedule_selector(Hero::reborn), 3.0f);
			}
			else
			{
				rebornLabelTime = getRebornTime();
				scheduleOnce(schedule_selector(Hero::reborn), getRebornTime());
			}
			if (!rebornSprite)
			{
				rebornSprite = Sprite::create();
				Sprite *skullSpirte = Sprite::createWithSpriteFrameName("skull.png");
				skullSpirte->setPosition(Vec2(0, 0));
				rebornSprite->addChild(skullSpirte);

				rebornLabel = CCLabelBMFont::create(to_cstr(rebornLabelTime), Fonts::Default);
				rebornLabel->setScale(0.3f);
				rebornLabel->setPosition(Vec2(skullSpirte->getContentSize().width, 0));
				rebornSprite->addChild(rebornLabel);

				rebornSprite->setPosition(Vec2(getContentSize().width / 2, getContentSize().height / 2));
				addChild(rebornSprite);
			}
			schedule(schedule_selector(Hero::countDown), 1);
		}
		// Keep the guardian body
	}

	void doAI()
	{
		CharacterBase::doAI();
		// Initial default AI handler
		aiHandler = std::bind(&Hero::perform, this);
	}

	/** Perform default AI logic (Hero::perform) */
	virtual void setAI(float dt)
	{
		// Not check nullable reference
		aiHandler();
		// proxy->perform(this);
	}

	/** Provide default AI logic */
	virtual void perform() = 0;

	/** Callback */

	/** Call on IGameModeHandler::onCharacterInit()
	 * @call_order:
	 * 	-> CharacterBase::setID()
	 * 	-> IGameModeHandler::onCharacterInit()
	 */

	virtual bool isEnableSkill04() { return true; }
	virtual bool isEnableSkill05() { return true; }

	template <typename THero>
	static typename std::enable_if<std::is_base_of<Hero, THero>::value, THero *>::type
	// typename enable_if<!is_same<Hero, THero>::value && is_base_of<Hero, THero>::value, THero *>::type
	create(const string &name, Role role, Group group)
	{
		THero *hero = new THero();
		if (hero->init())
		{
			hero->setID(name, role, group);
			hero->autorelease();
		}
		else
		{
			CCLOGERROR("Initial Hero %s error", typeof(THero));
			delete hero;
			return nullptr;
		}
		return hero;
	}

	template <typename THero>
	typename std::enable_if<std::is_base_of<Hero, THero>::value, THero *>::type
	createHero(const string &name, Role role) { return create<THero>(name, role, getGroup()); }

	template <typename THero>
	typename std::enable_if<std::is_base_of<Hero, THero>::value, THero *>::type
	createCloneHero(const string &name) { return createHero<THero>(name, Role::Clone); }

	template <typename THero>
	typename std::enable_if<std::is_base_of<Hero, THero>::value, THero *>::type
	createKugutsuHero(const string &name) { return createHero<THero>(name, Role::Kugutsu); }

	template <typename THero>
	typename std::enable_if<std::is_base_of<Hero, THero>::value, THero *>::type
	createSummonHero(const string &name) { return createHero<THero>(name, Role::Summon); }

/** Macros */
// eg.
//
// yourHero.hpp
//
// setAIHandler(yourHero::perform);						// Default	handler
// setAIHandler(yourHero::yourAIHandlerFunctionName);	// Custom	handler
#define setAIHandler(funcName) aiHandler = std::bind(&funcName, this)

	/** Character Macros */

#define match_char_exp(_name, _fn, _name2, _fn2) \
	if (getName() == _name)                      \
		_fn;                                     \
	else if (getName() == _name2)                \
		_fn2;

#define match_char_exp3(_name, _fn, _name2, _fn2, _name3, _fn3)            \
	match_char_exp(_name, _fn, _name2, _fn2) else if (getName() == _name3) \
		_fn3;

#define match_char_exp4(_name, _fn, _name2, _fn2, _name3, _fn3, _name4, _fn4)             \
	match_char_exp3(_name, _fn, _name2, _fn2, _name3, _fn3) else if (getName() == _name4) \
		_fn4;

protected:
	void checkRefCount(float dt)
	{
		CCLOG("[Ref Check] %s has %d references", getName().c_str(), retainCount());
	}

	void countDown(float dt)
	{
		rebornLabelTime -= 1;
		rebornLabel->setString(to_cstr(rebornLabelTime));
	}

	AIHandler aiHandler = nullptr;
};
