#pragma once
#include "Core/Utils/Parser.hpp"
#include "Data/UnitData.h"
#include "Effect.h"
#include "GameLayer.h"
#include "HudLayer.h"
#include "MyUtils/KTools.h"

enum class State : uint16_t
{
	DEAD,
	IDLE,
	WALK,
	NATTACK,
	SATTACK,	// Skills
	OATTACK,	// Ougi 1
	O2ATTACK,	// Ougi 2
	HURT,
	KNOCKDOWN,
	FLOAT,
	JUMP,
	AIRHURT
};

class HPBar;
class Hero;

class CharacterBase : public Sprite
{
friend class CommandSystem;

using ActionMap = Map<ActionFlag, FiniteTimeAction *>;

public:
	CharacterBase();

	virtual void		setID(const string &name, Role role, Group group);
	virtual void		setHPbar();
	virtual void		changeHPbar();
	virtual void		setShadows();

	PPROP(string, _name, Name);
	PPROP(Role, _role, Role);
	PPROP(Group, _group, Group);
	VPROP(int, _charId, CharId);

	VPROP(State, _state, State);

	uint32_t			_deadNum;
	uint32_t			_flogNum;
	PROP_UInt(_killNum, KillNum);
	PROP_UInt(_coin, Coin);


	CharacterBase*			_slayer;
	CharacterBase*			_sticker;
	RefPtr<CharacterBase>	_mainTarget;

	bool				isHurtingTower;

	bool				_isFlipped;
	bool				_isHitOne;
	bool				_isCatchOne;
	bool				_isHealing;
	bool				_isVisable;

	// buff
	uint32_t			_healBuffValue;
	uint32_t			_dehealBuffValue;
	uint32_t			_powerUPBuffValue;
	uint32_t			_skillUPBuffValue;
	uint32_t			_skillChangeBuffValue;


	Sprite*				_heartEffect;
	Effect*				_healBuffEffect;
	Effect*				_powerBuffEffect;
	Effect*				_skillBuffEffect;
	Sprite*				_dehealBuffEffect;
	Effect*				_healItemEffect;
	Effect*				_speedItemEffect;
	float				_buffStartTime;
	float				_debuffStartTime;
	VPROP(int, _knockLength, KnockLength);
	VPROP(bool, _knockDirection, KnockDirection);

	bool				_isAllAttackLocked;
	bool				_isOnlySkillLocked;
	bool				_isInvincible;
	bool				_isTaunt;
	// Armor : 霸体
	bool				_isArmored;
	bool				_isSuicide;

	bool				_isSticking;
	bool				_isPausing;

	Vec2				_markPoint;
	Vec2				_startPoint; // NOTE: Only for Minano

	bool				_isAI;
	bool				_isControlled;

	PROP_Vector(vector<GearType>, _gearArray, GearArray);

	bool				enableDead	 = true;
	bool				enableReborn = true;
	int					changeCharId = -1; // NOTE: Support for random deathmatch mode

	int					rebornLabelTime;
	int					battleCondiction;
	// True is only has one tower, false otherwise
	bool				isBaseDanger;


	HPBar*				_hpBar;
	Sprite*				_shadow;
	float				_originY;
	float				_backY;
	float				_diretionY;

	int					lbAttackerId = -1;
	uint32_t			lostBloodValue;
	int					gearCKRValue;
	uint32_t			gearRecoverValue;

	// 忍び：Kakuzu ability
	uint32_t			hearts;
	// Gear02 ability
	bool				isAttackGainCKR;
	// 口寄せ：Bikyu ability
	// 忍び：Guardian & Naruto-Rikutou & Naruto-Sage ability
	bool				hasArmorBroken;
	// Gear ability
	bool				hasArmor;
	bool				isGearCD;

	bool				_isCanGear00;
	bool				_isCanGear03;
	bool				_isCanGear06;

	VPROP(int,_walkSpeed,WalkSpeed);
	int					_originSpeed;

	PROP_UInt(_hp, HP);
	PROP_UInt(_maxHP, MaxHP);
	PROP_UInt(_ckr, CKR);
	PROP_UInt(_ckr2, CKR2);
	VPROP(int, _defense, DEF);
	VPROP(float, _exp, EXP);
	VPROP(uint32_t, _level, LV);
	VPROP(int, _height, Height);
	VPROP(int, _rebornTime, RebornTime);
	VPROP(Vec2, _spawnPoint, SpawnPoint);


	VPROP(CharacterBase*, _master, Master);
	VPROP(CharacterBase*, _controller, Controller);
	VPROP(CharacterBase*, _secmaster, SecMaster);

	PROP_REF(string,_attackType,AttackType);
	uint32_t		_attackValue;
	int				_attackRangeX;
	int				_attackRangeY;

	// int totalCombatPoint;
	int				enemyCombatPoint;
	int				friendCombatPoint;

	PROP_UInt(_nAttackValue,NAttackValue);
	PROP_REF(string,_nAttackType,NAttackType);
	int				_nAttackRangeX;
	int				_nAttackRangeY;

	PROP_UInt(_sAttackValue1,SAttackValue1);
	PROP_REF(string,_sAttackType1,SAttackType1);
	int				_sAttackRangeX1;
	int				_sAttackRangeY1;
	uint32_t		_sAttackCD1;
	bool			_sAttack1isDouble;
	int				_sAttackCombatPoint1;

	PROP_UInt(_sAttackValue2,SAttackValue2);
	PROP_REF(string,_sAttackType2,SAttackType2);
	int				_sAttackRangeX2;
	int				_sAttackRangeY2;
	uint32_t		_sAttackCD2;
	bool			_sAttack2isDouble;
	int				_sAttackCombatPoint2;

	PROP_UInt(_sAttackValue3,SAttackValue3);
	PROP_REF(string,_sAttackType3,SAttackType3);
	int				_sAttackRangeX3;
	int				_sAttackRangeY3;
	uint32_t		_sAttackCD3;
	bool			_sAttack3isDouble;
	int				_sAttackCombatPoint3;

	PROP_UInt(_sAttackValue4,SAttackValue4);
	PROP_REF(string,_sAttackType4,SAttackType4);
	int				_sAttackRangeX4;
	int				_sAttackRangeY4;
	uint32_t		_sAttackCD4;
	bool			_sAttack4isDouble;
	int				_sAttackCombatPoint4;

	PROP_UInt(_sAttackValue5,SAttackValue5);
	PROP_REF(string,_sAttackType5,SAttackType5);
	int				_sAttackRangeX5;
	int				_sAttackRangeY5;
	uint32_t		_sAttackCD5;
	int				_sAttackCombatPoint5;

	PROP_UInt(_spcAttackValue1,SpcAttackValue1);
	PROP_REF(string,_spcAttackType1,SpcAttack1Type);
	int				_spcAttackRangeX1;
	int				_spcAttackRangeY1;
	uint32_t		_spcAttackCD1;

	PROP_UInt(_spcAttackValue2,SpcAttackValue2);
	PROP_REF(string,_spcAttackType2,SpcAttack2Type);
	int				_spcAttackRangeX2;
	int				_spcAttackRangeY2;
	uint32_t		_spcAttackCD2;

	PROP_UInt(_spcAttackValue3,SpcAttackValue3);
	PROP_REF(string,_spcAttackType3,SpcAttack3Type);
	int				_spcAttackRangeX3;
	int				_spcAttackRangeY3;
	uint32_t		_spcAttackCD3;

	PROP_UInt(_tempAttackValue1,TempAttackValue1);


	PROP_REF(string, _effectType, EffectType);
	bool			_hurtFromLeft;
	bool			_hurtFromRight;

	int						_damageDisplayCount = 0;
	uint32_t				damageEffectCount;
	PROP_Vector(vector<CharacterBase *>, _monsterArray, MonsterArray);

	// movement
	PROP(Vec2, _velocity, Velocity);
	PROP(Vec2, _desiredPosition, DesiredPosition);

	// actions
	PPROP_PTR(FiniteTimeAction, _idleAction, IdleAction);
	PPROP_PTR(FiniteTimeAction, _nAttackAction, NAttackAction);
	PPROP_PTR(FiniteTimeAction, _walkAction, WalkAction);
	PPROP_PTR(FiniteTimeAction, _hurtAction, HurtAction);
	PPROP_PTR(FiniteTimeAction, _knockDownAction, KnockDownAction);
	PPROP_PTR(FiniteTimeAction, _airHurtAction, AirHurtAction);
	PPROP_PTR(FiniteTimeAction, _floatAction, FloatAction);
	PPROP_PTR(FiniteTimeAction, _deadAction, DeadAction); 

	PPROP_PTR(FiniteTimeAction, _skill1Action, Skill1Action);
	PPROP_PTR(FiniteTimeAction, _skill2Action, Skill2Action);
	PPROP_PTR(FiniteTimeAction, _skill3Action, Skill3Action);
	PPROP_PTR(FiniteTimeAction, _skill4Action, Skill4Action);
	PPROP_PTR(FiniteTimeAction, _skill5Action, Skill5Action);

	PPROP_PTR(FiniteTimeAction, _moveAction, MoveAction);
	PPROP_PTR(FiniteTimeAction, _floatUPAction, FloatUPAction);
	PPROP_PTR(FiniteTimeAction, _jumpUPAction, JumpUPAction);


	void				update(float dt);
	void				updateHpBarPosition(float dt);
	void				acceptAttack(Ref* object);

	float				getHpPercent();

	bool				checkHasMovement();

	void				idle();
	void				walk(Vec2 direction);
	void				attack(ABType type);
	void				sAttack(ABType type);
	void				nAttack();
	void				oAttack(ABType type);
	bool				hurt();
	bool				hardHurt(int delayTime, bool isHurtAction, bool isCatch, bool isStick, bool isStun);
	void				sendOnlineKnockSnapToPeer(float dt);
	// bool				hardHurt(int delayTime, bool isHurtAction, HardHurtState state);
	void				absorb(Vec2 position, bool isImmediate);
	void				jump() {}; // No reference
	void				knockDown();
	virtual void		dead();
	void				floatUP(float floatHeight, bool isCancelSkill);
	void				airHurt();

	virtual void		doAI();


	virtual void		changeAction();
	void				changeAction2();
	void				changeGroup();
private:
	template <typename T>
	typename std::enable_if<std::is_base_of<CharacterBase, T>::value, void>::type
	changeGroupBy(const vector<T *> &list);
public:
	virtual void		resumeAction(float dt);
	virtual void		setActionResume();
	void				setActionResume2();
	void				reCatched(float dt);

	void				setShadow(SpriteFrame *frame);
	void				enableShadow(Sprite *charN);
	void				disableShadow(Sprite *charN);

	void				setItem(ABType type);
	void				disableEffect();
	void				disableHpBar(float dt);

	void				disableShack();
	void				disableDebuff(float dt);

	void				enableSkill1(float dt);
	void				enableSkill2(float dt);
	void				enableSkill3(float dt);
	void				enableItem1(float dt);

	void				enableGear00(float dt);
	void				enableGear03(float dt);
	void				enableGear06(float dt);

	void				disableGear1(float dt);
	void				disableGear2(float dt);
	void				disableGear3(float dt);

	bool				setGear(GearType type);
	void				useGear(GearType type);

	void				setCoinDisplay(int num);
	void				removeCoinDisplay(Sprite *coinDisplay);
	void				addCoin(uint32_t num);
	void				minusCoin(uint32_t num);

	bool				_isCanSkill1;
	bool				_isCanSkill2;
	bool				_isCanSkill3;
	bool				_isCanItem1;
	bool				_isCanOugis1;
	bool				_isCanOugis2;

	CCArray*			idleArray;
	CCArray*			walkArray;
	CCArray*			hurtArray;
	CCArray*			airHurtArray;
	CCArray*			knockDownArray;
	CCArray*			floatArray;
	CCArray*			deadArray;
	CCArray*			nattackArray;
	CCArray*			skill1Array;
	CCArray*			skill2Array;
	CCArray*			skill3Array;
	CCArray*			skill4Array;
	CCArray*			skill5Array;

	CCArray*			skillSPC1Array;
	CCArray*			skillSPC2Array;
	CCArray*			skillSPC3Array;
	CCArray*			skillSPC4Array;
	CCArray*			skillSPC5Array;


	virtual void		dealloc();
	virtual void		reborn(float dt);


	void				resumePauseStuff(float dt);

	void				lostBlood(float dt);
	void				removeLostBlood(float dt);

	virtual void		setAI(float dt);
	virtual void		setRestore2(float dt);


	void				readData(CCArray* tmpData, string &attackType, uint32_t &attackValue, int &attackRangeX, int &attackRangeY, uint32_t &cooldown, int &combatPoint);
	FiniteTimeAction*	createAnimation(CCArray* ationArray, uint8_t fps, bool isLoop, bool isReturnToIdle);
	FiniteTimeAction*	createAnimation(CCArray* arr, const ActionConstant::AnimationInfo &info) { return createAnimation(arr, info.fps, info.isLoop, info.isReturnToIdle); }
	FiniteTimeAction*	createAnimIdle(CCArray* arr) { return createAnimation(arr, ActionConstant::Idle); }
	FiniteTimeAction*	createAnimKnockdown(CCArray* arr) { return createAnimation(arr, ActionConstant::Knockdown); }
	FiniteTimeAction*	createAnimRegular(CCArray* arr) { return createAnimation(arr, ActionConstant::Regular); }
	FiniteTimeAction*	createAnimSkill(CCArray* arr) { return createAnimation(arr, ActionConstant::Skill); }

	void				setSound(const string &file);
	void				setDSound(const string &file);
	void				setMove(int moveLength);
	void				setCharge(int moveLength);
	void				setChargeB(int moveLength);
	void				getCollider();
	void				getSticker(float dt);
	void				stopMove(float dt);
	void				stopJump(int stopTime);
	void				setCharFlip();
	void				setAttackBox(const string &effectType);
	inline void			setDamage(CharacterBase* attacker);
	void				setDamage(CharacterBase *attacker, const string &effectType, int attackValue, bool isFlipped);
	void				setDamgeDisplay(int value, const char* font);

	void				setSkillEffect(const string &type);
	// void				setItemEffect(const string &type);
	void				setDamgeEffect(const string &type);
	void				setMonAttack(int skillNum);
	void				setTransform();
	void				setOugis();

	virtual Hero*		createClone(int cloneTime);
	void				setClone(int cloneTime);
	void				setMon(const string &monName);
	void				setMonPer(float dt);
	void				setBullet(const string &bulletName);
	void				setBulletGroup(float dt);
	void				setTrap(const string &trapName);

	void				removeAllClones();

	void				setBuff(int buffValue);
	void				setCommand(const string &cmd);
	void				setBuffEffect(const string &type);
	void				removeBuffEffect(const string &type);

	void				disableBuff(float dt);
	void				healBuff(float dt);
	void				dehealBuff(float dt);

protected:
	void				removeClone(float dt);
	void				removeSelf(float dt);
	void				setJump(bool jumpDirection);

	CCRect				setHalfBox();
	CCRect				setHitBox();

	void				checkActionFinish(float dt);

	void				setRestore(float dt);


	bool				findEnemy(Role role, int searchRange, bool masterRange = false);
	bool				findEnemy2(Role role);
	bool				findTargetEnemy(Role role, bool isTowerDected);
	template <typename T>
	typename std::enable_if<std::is_base_of<CharacterBase, T>::value, bool>::type
	findEnemyBy(const vector<T *> &list, int searchRange, bool masterRange = false);
	template <typename T>
	typename std::enable_if<std::is_base_of<CharacterBase, T>::value, bool>::type
	findEnemy2By(const vector<T *> &list);
	template <typename T>
	typename std::enable_if<std::is_base_of<CharacterBase, T>::value, bool>::type
	findTargetEnemyBy(const vector<T *> &list, bool isTowerDected);
	bool				checkBase();

	bool				stepBack();
	bool				stepBack2();
	bool				checkRetri();
	void				stepOn();
	void				changeSide(Vec2 sp);

	inline void			autoFlip(CharacterBase* attacker);

	void				removeDamageDisplay(CCNode *damageDisplay);

public:
	bool hasTempAttackValue1() { return getTempAttackValue1() > 0; }
	// UI
	void updateHpBar();
	void updateDataByLVOnly();
	void lockSkill4Button() {
		if (isPlayer())
			getGameLayer()->getHudLayer()->skill4Button->setLock();
	}
	void unlockSkill4Button() {
		if (isPlayer())
			getGameLayer()->getHudLayer()->skill4Button->unLock();
	}
	void lockSkill5Button() {
		if (isPlayer())
			getGameLayer()->getHudLayer()->skill5Button->setLock();
	}
	void unlockSkill5Button() {
		if (isPlayer())
			getGameLayer()->getHudLayer()->skill5Button->unLock();
	}
	void lockOugisButtons() {
		if (isPlayer()) {
			getGameLayer()->getHudLayer()->skill4Button->setLock();
			getGameLayer()->getHudLayer()->skill5Button->setLock();
		}
	}
	void unlockOugisButtons() {
		if (isPlayer()) {
			getGameLayer()->getHudLayer()->skill4Button->unLock();
			getGameLayer()->getHudLayer()->skill5Button->unLock();
		}
	}
	// character extensions
	void setHPValue(uint32_t var, bool isUpdateHpBar = true) {
		setHP(var);
		if (isUpdateHpBar)
			updateHpBar();
	}
	void setMaxHPValue(uint32_t var, bool isUpdateHpBar = true) {
		setMaxHP(var);
		if (isUpdateHpBar)
			updateHpBar();
	}
	void resetDefenseValue(int def) {
		if (_defense > 0 && _defense > def)
			_defense -= def;
		else
			_defense = 0;
	}
	bool isGuardian() { return _name == GuardianEnum::Han || _name == GuardianEnum::Roshi; }
	bool isNotGuardian() { return !isGuardian(); }
	// role extensions
	bool isCom() { return _role == Role::Com; }
	bool isPlayer() { return _role == Role::Player; }
	bool isPlayerOrCom() { return isPlayer() || isCom(); }
	bool isFlog() { return _role == Role::Flog; }
	bool isTower() { return _role == Role::Tower; }
	bool isBullet() { return _role == Role::Bullet; }
	bool isClone() { return _role == Role::Clone; }
	bool isKugutsu() { return _role == Role::Kugutsu; }
	bool isMon() { return _role == Role::Mon; }
	bool isSummon() { return _role == Role::Summon; }
	bool isNotCom() { return _role != Role::Com; }
	bool isNotPlayer() { return _role != Role::Player; }
	bool isNotFlog() { return _role != Role::Flog; }
	bool isNotTower() { return _role != Role::Tower; }
	bool isNotBullet() { return _role != Role::Bullet; }
	bool isNotClone() { return _role != Role::Clone; }
	bool isNotKugutsu() { return _role != Role::Kugutsu; }
	bool isNotMon() { return _role != Role::Mon; }
	bool isNotSummon() { return _role != Role::Summon; }
	// group extensions
	bool isKonohaGroup() { return _group == Group::Konoha; }
	bool isAkatsukiGroup() { return _group == Group::Akatsuki; }
	// monster extensions
	bool hasMonsterArrayAny() { return !_monsterArray.empty(); }
	void removeMon(CharacterBase *mo) {
		if (hasMonsterArrayAny())
			std::erase(_monsterArray, mo);
	}
	void removeAllMonAndCleanup(const string &name) {
		if (hasMonsterArrayAny()) {
			for (auto mo : _monsterArray)
			{
				if (mo->getName() == name)
				{
					std::erase(_monsterArray, mo);
					mo->removeFromParent();
				}
			}
		}
	}
	// State extensions

	// State is IDLE, WALK or NATTACK
	bool isFreeState() {
		return _state == State::IDLE || _state == State::WALK || _state == State::NATTACK;
	}
	// Action extensions
	bool hasAction(ActionFlag action) {
		return (_actionFlag & action) != ActionFlag::None;
	}
	// Utilities
	void increaseAllCkrs(uint32_t value, bool enableLv2 = true, bool enableLv4 = true);
	void increaseHpAndUpdateUI(uint32_t value);
protected:
	// find enemy extensions
	bool findHero(int searchRange, bool masterRange = false) { return findEnemy(Role::Hero, searchRange, masterRange); }
	bool findFlog(int searchRange, bool masterRange = false) { return findEnemy(Role::Flog, searchRange, masterRange); }
	bool findTower(int searchRange, bool masterRange = false) { return findEnemy(Role::Tower, searchRange, masterRange); }
	bool notFindHero(int searchRange, bool masterRange = false) { return !findEnemy(Role::Hero, searchRange, masterRange); }
	bool notFindFlog(int searchRange, bool masterRange = false) { return !findEnemy(Role::Flog, searchRange, masterRange); }
	bool notFindTower(int searchRange, bool masterRange = false) { return !findEnemy(Role::Tower, searchRange, masterRange); }
	bool findHeroHalf() { return findEnemy2(Role::Hero); }
	bool findFlogHalf() { return findEnemy2(Role::Flog); }
	bool findTowerHalf() { return findEnemy2(Role::Tower); }
	bool notFindHeroHalf() { return !findEnemy2(Role::Hero); }
	bool notFindFlogHalf() { return !findEnemy2(Role::Flog); }
	bool notFindTowerHalf() { return !findEnemy2(Role::Tower); }
	// AI extensions
	void tryBuyGear(GearType gear1, GearType gear2, GearType gear3) {
		if (getCoin() >= 500 && getGearArray().size() < 3 && !_isControlled && getGameLayer()->_enableGear)
		{
			if (getGearArray().size() == 0)
				setGear(gear1);
			else if (getGearArray().size() == 1)
				setGear(gear2);
			else if (getGearArray().size() == 2)
				setGear(gear3);
		}
	}
	void tryUseGear6() {
		if (_isCanGear06)
		{
			if ((_state == State::FLOAT ||
				 _state == State::AIRHURT ||
				 _state == State::HURT ||
				 _state == State::KNOCKDOWN) &&
				getHpPercent() < 0.5 && !_isArmored && !_isInvincible)
			{
				useGear(GearType::Gear06);
			}
		}
	}
	bool needBackToTowerToRestoreHP(bool isNeedBack = true) {
		if (checkRetri() && isNeedBack)
		{
			if (_mainTarget)
			{
				if (stepBack2())
				{
					if (_isCanGear00 && !_isArmored)
						useGear(GearType::Gear00);
					return true;
				}
			}
			else
			{
				if (_isCanGear00)
					useGear(GearType::Gear00);

				if (stepBack())
					return true;
			}
		}
		return false;
	}
	bool needBackToDefendTower(bool isNeedBack = true) {
		if (isBaseDanger && checkBase() && !_isControlled && isNeedBack)
		{
			bool needBack = false;
			if (isAkatsukiGroup())
			{
				if (getPositionX() < 85 * 32)
					needBack = true;
			}
			else
			{
				if (getPositionX() > 11 * 32)
					needBack = true;
			}

			if (needBack)
			{
				if (_isCanGear00)
					useGear(GearType::Gear00);

				if (stepBack2())
					return true;
			}
		}
		return false;
	}
	bool checkMove(float x = 32, float y = 32) {
		auto sp = getDistanceToTarget();
		if (abs(sp.x) > x || abs(sp.y) > y)
		{
			auto moveDirection = sp.getNormalized();
			walk(moveDirection);
			return true;
		}
		return false;
	}
	void checkHealingState() {
		if (_isHealing && getHpPercent() < 1)
		{
			if (isFreeState())
				idle();
		}
		else
		{
			if (_isCanGear00 && !_isArmored)
				useGear(GearType::Gear00);

			stepOn();
		}
	}
	Vec2 getDirByMoveTo(CharacterBase *target) {
		return (target->getPosition() - getPosition()).getNormalized();
	}
	Vec2 getDistanceToTarget() {
		return _mainTarget->_originY
			? Vec2(_mainTarget->getPositionX(), _mainTarget->_originY) - getPosition()
			: _mainTarget->getPosition() - getPosition();
	}
	Vec2 getDistanceToTargetAndIgnoreOriginY() {
		return _mainTarget->getPosition() - getPosition();
	}

/**
 * Callbacks
 */
protected:
	virtual bool onAcceptAttack(CharacterBase *attacker) { return true; }
	// Returns true will call CharacterBase::setDamage(attacker), false otherwise.
	virtual bool onHit(CharacterBase *attacker) { return true; }
	// Returns true will call CharacterBase::setDamage(attacker), false otherwise.
	virtual bool onBulletHit(CharacterBase *attacker) { return true; }

	virtual void onSetTrap(const string &trapType) { }

	void clearActionData() {
		_actionFlag = ActionFlag::None;
		_actionMap.clear();
	}

private:
	ActionFlag	_actionFlag;
	ActionMap	_actionMap;

	bool _affectedByTower;
};
