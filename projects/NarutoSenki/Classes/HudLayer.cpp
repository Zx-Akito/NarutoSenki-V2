#include "Core/Hero.hpp"
#include "GameLayer.h"
#include "HudLayer.h"
#include "Utils/Cocos2dxHelper.hpp"
#include "MyUtils/CCShake.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
extern "C" bool MacWsIsConnected();
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
extern "C" bool NativeBridgeWsIsConnected(void);
#endif
extern "C" int GetNetworkForcedTeam();
void NetLatencyPingSend();

static inline bool hudOnlineWsConnected()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	return MacWsIsConnected();
#else
	return NativeBridgeWsIsConnected();
#endif
}
#endif

MiniIcon::MiniIcon()
{
}

MiniIcon::~MiniIcon()
{
	CCNotificationCenter::sharedNotificationCenter()->removeObserver(this, "updateMap");
}

bool MiniIcon::init(const char *szImage, bool isNotification)
{
	RETURN_FALSE_IF(!Sprite::init());

	initWithSpriteFrameName(szImage);

	if (isNotification)
	{
		setAnchorPoint(Vec2(0.5f, 0));
		CCNotificationCenter::sharedNotificationCenter()->addObserver(this, callfuncO_selector(MiniIcon::updateMap), "updateMap", nullptr);
	}
	else
	{
		setAnchorPoint(Vec2(0.5f, 0.5f));
	}

	return true;
}

void MiniIcon::updateMap(Ref *sender)
{
	auto poster = (CharacterBase *)sender;

	if (getCharId() == poster->getCharId())
	{
		if (poster->getState() == State::DEAD || poster->_isVisable == false)
		{
			setVisible(false);
		}
		else
		{
			setVisible(true);
			updatePosition(poster->getPosition());
		}
	}
}

void MiniIcon::updateState()
{
	auto frame = getSpriteFrame("tower_icon3.png");
	setDisplayFrame(frame);
}

void MiniIcon::updatePosition(Vec2 location)
{
	int x = floor(location.x / 32);
	int y = floor(location.y / 32) * 5;

	setPosition(Vec2(x, y));
	getGameLayer()->reorderChild(this, 500);
}

MiniIcon *MiniIcon::create(const char *szImage, bool isNotification)
{
	MiniIcon *mi = new MiniIcon();
	if (mi && mi->init(szImage, isNotification))
	{
		mi->autorelease();
		return mi;
	}
	else
	{
		delete mi;
		return nullptr;
	}
}

HudLayer::HudLayer()
{
	status_bar = nullptr;
	status_hpbar = nullptr;
	status_hpMark = nullptr;
	status_expbar = nullptr;
	nAttackButton = nullptr;
	skill1Button = nullptr;
	skill2Button = nullptr;
	skill3Button = nullptr;
	gearMenuSprite = nullptr;
	reportSprite = nullptr;
	hpLabel = nullptr;
	expLabel = nullptr;
	coinLabel = nullptr;
	killLabel = nullptr;
	deadLabel = nullptr;
	KonoLabel = nullptr;
	AkaLabel = nullptr;
	gameClock = nullptr;
	pauseNenu = nullptr;
	texUI = nullptr;
	uiBatch = nullptr;
	miniLayer = nullptr;
	_joyStick = nullptr;

	reportSPCSprite = nullptr;

	_isAllButtonLocked = false;

	skill4Button = nullptr;
	skill5Button = nullptr;
	ougisLayer = nullptr;

	gear1Button = nullptr;
	gear2Button = nullptr;
	gear3Button = nullptr;
	gearMenu = nullptr;

	item1Button = nullptr;
	item2Button = nullptr;
	item3Button = nullptr;
	item4Button = nullptr;
	openingSprite = nullptr;

	bcdLabel1 = nullptr;
	bcdLabel2 = nullptr;
	pingLabel = nullptr;
}

void HudLayer::onEnter()
{
	if (getGameLayer()->_isExiting)
	{
		return;
	}
	Layer::onEnter();
}

void HudLayer::onExit()
{
	unschedule(schedule_selector(HudLayer::tickOnlinePingLatency));
	Layer::onExit();
	if (getGameLayer()->_isExiting)
	{
		_reportListArray.clear();
		_towerIconArray.clear();
	}
}

bool HudLayer::init()
{
	RETURN_FALSE_IF(!Layer::init());

	texUI = TextureCache::sharedTextureCache()->addImage("UI.png");
	// TODO: Remove batch node when upgrading to cocos2d-x V4
	uiBatch = SpriteBatchNode::createWithTexture(texUI);
	addChild(uiBatch);

	return true;
}

void HudLayer::JoyStickRelease()
{
	getGameLayer()->JoyStickRelease();
}

void HudLayer::JoyStickUpdate(Vec2 direction)
{
	getGameLayer()->JoyStickUpdate(direction);
}

void HudLayer::initGearButton(const string &charName)
{
	gearMenuSprite = MenuItemSprite::create(Sprite::createWithSpriteFrameName(format("{}_avator.png", charName).c_str()), nullptr, nullptr, this, menu_selector(HudLayer::gearButtonClick));
	if (gearMenuSprite == nullptr)
	{
		CCLOGERROR("Not found %s avator sprite", charName.c_str());
		return;
	}

	if (gearMenu)
		gearMenu->removeFromParent();
	gearMenuSprite->setAnchorPoint(Vec2(0, 0));
	gearMenu = Menu::create(gearMenuSprite, nullptr);
	gearMenu->setPosition(Vec2(0, winSize.height - gearMenuSprite->getContentSize().height));
	gearMenu->setTouchPriority(10);
	addChild(gearMenu, 100);
}

void HudLayer::playGameOpeningAnimation()
{
	Vector<SpriteFrame *> spriteFrames;
	int i = 1;
	while (i < kComCount)
	{
		auto frame = getSpriteFrame("gameStart_00{}.png", i);
		spriteFrames.pushBack(frame);
		i += 1;
	}

	auto tempAnimation = Animation::createWithSpriteFrames(spriteFrames, 0.1f);
	auto tempAction = Animate::create(tempAnimation);

	openingSprite = Sprite::createWithSpriteFrameName("gameStart_001.png");
	openingSprite->setPosition(Vec2(winSize.width / 2 + 32, winSize.height / 2));
	addChild(openingSprite, 5000);
	openingSprite->runAction(tempAction);
}

void HudLayer::initHeroInterface()
{
	auto winWidth = winSize.width;
	auto winHeight = winSize.height;
	float mScaleX = 1.0f;
	float mScaleY = 1.0f;

#if DEBUG || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS
	mScaleX = winWidth / 570;
	mScaleY = winHeight / 320;
	// init the joyStick
	_joyStick = JoyStick::create();
	// set relative position in large screen and small screen
	_joyStick->setPosition(Vec2(mScaleX * 28, mScaleY * 16));
	_joyStick->setDelegate(this);
	uiBatch->addChild(_joyStick);
#endif

	// produce status bar
	status_bar = Sprite::createWithSpriteFrameName("status_bar_bg.png");
	status_bar->setAnchorPoint(Vec2(0, 0));
	status_bar->setPosition(Vec2(0, winHeight - status_bar->getContentSize().height));
	uiBatch->addChild(status_bar);

	status_hpbar = Sprite::createWithSpriteFrameName("status_hpbar.png");
	status_hpbar->setPosition(Vec2(53, winHeight - 54));
	addChild(status_hpbar, 40);

	status_hpMark = Sprite::createWithSpriteFrameName("status_hpMark.png");
	status_hpMark->setAnchorPoint(Vec2(0, 0));
	status_hpMark->setPosition(Vec2(54, winHeight - 105));
	addChild(status_hpMark, 45);

	Sprite *tmpSprite = Sprite::createWithSpriteFrameName("status_ckrbar.png");
	status_expbar = ProgressTimer::create(tmpSprite);
	status_expbar->setType(kCCProgressTimerTypeRadial);
	status_expbar->setPercentage(0);
	status_expbar->setReverseDirection(true);
	status_expbar->setPosition(Vec2(54, winHeight - 54));
	addChild(status_expbar, 50);
	MenuItem *menu_button = MenuItemSprite::create(Sprite::createWithSpriteFrameName("minimap_bg.png"), nullptr, nullptr, this, menu_selector(HudLayer::pauseButtonClick));
	menu_button->setAnchorPoint(Vec2(1, 1));
	pauseNenu = Menu::create(menu_button, nullptr);
	pauseNenu->setPosition(Vec2(winWidth, winHeight));
	addChild(pauseNenu);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	// Match config is set before LoadLayer; WS may report connected slightly later on Android (JNI),
	// so key off network session, not only hudOnlineWsConnected() at init time.
	if (GetNetworkForcedTeam() != -1 || hudOnlineWsConnected())
	{
		const float mmw = menu_button->getContentSize().width;
		pingLabel = CCLabelBMFont::create("-- ms", Fonts::Default);
		pingLabel->setScale(0.22f);
		pingLabel->setAnchorPoint(Vec2(1.f, 1.f));
		// Beside minimap_bg icon (pause/minimap button), not the tactical miniLayer.
		pingLabel->setPosition(Vec2(winWidth - mmw - 8.f, winHeight - 6.f));
		addChild(pingLabel, 5002);
		tickOnlinePingLatency(0.f);
		schedule(schedule_selector(HudLayer::tickOnlinePingLatency), 2.0f);
	}
#endif

	Sprite *killIcon = Sprite::createWithSpriteFrameName("kill_icon.png");
	killIcon->setAnchorPoint(Vec2(0, 1));
	killIcon->setPosition(Vec2(winWidth - 114, winHeight - 46));
	addChild(killIcon, 5000);

	killLabel = CCLabelBMFont::create("0", Fonts::Default);
	killLabel->setScale(0.26f);
	killLabel->setAnchorPoint(Vec2(0, 1));
	killLabel->setPosition(Vec2(killIcon->getPositionX() + killIcon->getContentSize().width / 2 + 5, winHeight - 56));
	addChild(killLabel, 5001);

	Sprite *deadIcon = Sprite::createWithSpriteFrameName("dead_icon.png");
	deadIcon->setAnchorPoint(Vec2(0, 1));
	deadIcon->setPosition(Vec2(winWidth - 114 + 26, winHeight - 47));
	addChild(deadIcon, 5000);

	deadLabel = CCLabelBMFont::create("0", Fonts::Default);
	deadLabel->setScale(0.26f);
	deadLabel->setAnchorPoint(Vec2(0, 1));
	deadLabel->setPosition(Vec2(deadIcon->getPositionX() + deadIcon->getContentSize().width / 2 + 5, winHeight - 56));
	addChild(deadLabel, 5001);

	KonoLabel = CCLabelBMFont::create("0", Fonts::Blue);
	KonoLabel->setScale(0.35f);

	AkaLabel = CCLabelBMFont::create("0", Fonts::Red);
	AkaLabel->setScale(0.35f);

	if (getGameLayer()->playerGroup == Group::Konoha)
	{
		KonoLabel->setAnchorPoint(Vec2(1, 1));
		AkaLabel->setAnchorPoint(Vec2(0, 1));
		KonoLabel->setPosition(Vec2(winWidth - 42, winHeight - 46));
		AkaLabel->setPosition(Vec2(winWidth - 32, winHeight - 46));
	}
	else
	{
		AkaLabel->setAnchorPoint(Vec2(1, 1));
		KonoLabel->setAnchorPoint(Vec2(0, 1));
		AkaLabel->setPosition(Vec2(winWidth - 42, winHeight - 46));
		KonoLabel->setPosition(Vec2(winWidth - 32, winHeight - 46));
	}

	addChild(KonoLabel, 5000);
	addChild(AkaLabel, 5000);
	auto hengLabel = CCLabelBMFont::create(":", Fonts::Default);
	hengLabel->setScale(0.5f);
	hengLabel->setAnchorPoint(Vec2(0.5f, 1));
	hengLabel->setPosition(Vec2(winWidth - 37, KonoLabel->getPositionY() + 2));
	addChild(hengLabel, 5000);

	auto currentPlayer = getGameLayer()->currentPlayer;
	auto currCharName = currentPlayer->getName();
	initGearButton(currCharName);

	hpLabel = CCLabelBMFont::create(currentPlayer->getHP_Value().asString().c_str(), Fonts::Default);
	hpLabel->setScale(0.35f);
	hpLabel->setPosition(Vec2(0, winHeight - 54));
	hpLabel->setAnchorPoint(Vec2(0, 0));

	addChild(hpLabel, 5000);

	int exp = currentPlayer->getEXP() - ((currentPlayer->getLV() - 1) * 500) / 500;
	expLabel = CCLabelBMFont::create(format("{}%", exp).c_str(), Fonts::Default);
	expLabel->setScale(0.35f);
	expLabel->setPosition(Vec2(94, winHeight - 54));
	expLabel->setAnchorPoint(Vec2(0.5f, 0));

	addChild(expLabel, 5000);

	gameClock = CCLabelBMFont::create("00:00", Fonts::Default);
	gameClock->setAnchorPoint(Vec2(0.5f, 0));
	gameClock->setScale(0.4f);
	gameClock->setPosition(Vec2(mScaleX * 25, 0));
	addChild(gameClock, 5000);

	coinLabel = CCLabelBMFont::create(formatCoinForDisplay(static_cast<int>(currentPlayer->getCoin())).c_str(), Fonts::Arial);
	coinLabel->setAnchorPoint(Vec2(0, 0));
	coinLabel->setPosition(Vec2(121, winHeight - 61));

	addChild(coinLabel, 5000);

	// init NATTACK Button
	nAttackButton = ActionButton::create("n_attack.png");
	nAttackButton->setDelegate(this);
	nAttackButton->setABType(NAttack);
	uiBatch->addChild(nAttackButton);

	// init SKIll Button

	skill1Button = ActionButton::create(currCharName + "_skill1.png");
	skill1Button->setDelegate(this);
	skill1Button->setABType(SKILL1);
	uiBatch->addChild(skill1Button);

	skill2Button = ActionButton::create(currCharName + "_skill2.png");
	skill2Button->setDelegate(this);
	skill2Button->setABType(SKILL2);
	uiBatch->addChild(skill2Button);

	skill3Button = ActionButton::create(currCharName + "_skill3.png");
	skill3Button->setDelegate(this);
	skill3Button->setABType(SKILL3);
	uiBatch->addChild(skill3Button);

	skill4Button = ActionButton::create(currCharName + "_skill4.png");
	skill4Button->setDelegate(this);
	skill4Button->setABType(OUGIS1);
	uiBatch->addChild(skill4Button);

	skill5Button = ActionButton::create(currCharName + "_skill5.png");
	skill5Button->setDelegate(this);
	skill5Button->setABType(OUGIS2);
	uiBatch->addChild(skill5Button);

	// init Item
	item1Button = ActionButton::create("item1.png");
	if (getGameLayer()->_isHardCoreGame)
	{
		item1Button->setCD(5000);
	}
	else
	{
		item1Button->setCD(3000);
	}

	item1Button->setDelegate(this);
	item1Button->setABType(Item1);
	item1Button->_cost = "50";
	uiBatch->addChild(item1Button);

	// init Item
	item2Button = ActionButton::create("item2.png");
	item2Button->setCD(15000);
	item2Button->setDelegate(this);
	item2Button->setABType(GearItem);
	item2Button->_gearType = GearType::None;
	item2Button->_isLock = true;
	uiBatch->addChild(item2Button);
	item2Button->setVisible(false);

	item3Button = ActionButton::create("item3.png");
	item3Button->setCD(15000);
	item3Button->setDelegate(this);
	item3Button->setABType(GearItem);
	item3Button->_gearType = GearType::None;
	item3Button->_isLock = true;
	uiBatch->addChild(item3Button);
	item3Button->setVisible(false);

	item4Button = ActionButton::create("item4.png");
	item4Button->setCD(15000);
	item4Button->setDelegate(this);
	item4Button->setABType(GearItem);
	item4Button->_gearType = GearType::None;
	item4Button->_isLock = true;
	uiBatch->addChild(item4Button);
	item4Button->setVisible(false);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID) || (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	nAttackButton->setPosition(Vec2(winWidth - 60, 8));
	skill1Button->setPosition(Vec2(winWidth - skill1Button->getContentSize().width - 64, 2));
	skill2Button->setPosition(Vec2(winWidth - 95, 50));
	skill3Button->setPosition(Vec2(winWidth - 44, 8 + nAttackButton->getContentSize().height + 8));
	skill4Button->setPosition(Vec2(skill1Button->getPositionX() - skill4Button->getContentSize().width - 8, 2));
	skill5Button->setPosition(Vec2(skill4Button->getPositionX() - skill5Button->getContentSize().width - 8, 2));

	item1Button->setPosition(Vec2(8, skill3Button->getPositionY() + skill3Button->getContentSize().height));
	item2Button->setPosition(Vec2(winWidth - 44, skill3Button->getPositionY() + skill3Button->getContentSize().height + 8));
	if (skill5Button)
		item3Button->setPosition(Vec2(skill5Button->getPositionX() - skill5Button->getContentSize().width - 8, 2));
	else if (skill4Button)
		item3Button->setPosition(Vec2(skill4Button->getPositionX() - skill4Button->getContentSize().width - 8, 2));
	else
		item3Button->setPosition(Vec2(skill1Button->getPositionX() - skill1Button->getContentSize().width - 8, 2));
	item4Button->setPosition(Vec2(skill2Button->getPositionX() - skill2Button->getContentSize().width - 8, skill2Button->getPositionY()));
#else
	float width = skill1Button->getContentSize().width;

	nAttackButton->setPosition(Vec2(winWidth + 100, -100));

	skill1Button->setPosition(Vec2(winWidth / 2 - (width + 8) * 2, 2));
	skill1Button->setScale(DESKTOP_UI_SCALE);
	skill2Button->setPosition(Vec2(winWidth / 2 - width - 8, 2));
	skill2Button->setScale(DESKTOP_UI_SCALE);
	skill3Button->setPosition(Vec2(winWidth / 2, 2));
	skill3Button->setScale(DESKTOP_UI_SCALE);
	skill4Button->setPosition(Vec2(winWidth / 2 + width + 8, 2));
	skill4Button->setScale(DESKTOP_UI_SCALE);
	skill5Button->setPosition(Vec2(winWidth / 2 + (width + 8) * 2, 2));
	skill5Button->setScale(DESKTOP_UI_SCALE);

	item1Button->setScale(DESKTOP_UI_SCALE);
	item2Button->setScale(DESKTOP_UI_SCALE);
	item3Button->setScale(DESKTOP_UI_SCALE);
	item4Button->setScale(DESKTOP_UI_SCALE);

	// Ramen	Item
	item1Button->setPosition(Vec2(8, 8 + nAttackButton->getContentSize().height + skill1Button->getContentSize().height));
	// First 	Item (Speed Up & Stealth)
	item3Button->setPosition(Vec2(winWidth - skill1Button->getContentSize().width - 64, 2));
	// Second 	Item (変わり身の術)
	item4Button->setPosition(Vec2(winWidth - 95, 50));
	// Third 	Item (Trap)
	item2Button->setPosition(Vec2(winWidth - 44, 8 + nAttackButton->getContentSize().height + 8));
#endif

	// Set mask
	skill1Button->setMarkSprite("skill_freeze.png");
	skill2Button->setMarkSprite("skill_freeze.png");
	skill3Button->setMarkSprite("skill_freeze.png");
	skill4Button->setOugisMark();
	skill5Button->setOugisMark();

	item1Button->setMarkSprite("skill_freeze.png");
	item2Button->setMarkSprite("skill_freeze.png");
	item3Button->setMarkSprite("skill_freeze.png");
	item4Button->setMarkSprite("skill_freeze.png");

	// init gear
	updateGears();
	miniLayer = Layer::create();
	miniLayer->setAnchorPoint(Vec2(0, 0));
	miniLayer->setPosition(Vec2(winWidth - 112, winHeight - 38));

	for (auto tower : getGameLayer()->_TowerArray)
	{
		const char *path = "";
		MiniIcon *mi;
		if (tower->getGroup() == currentPlayer->getGroup())
			path = "tower_icon1.png";
		else
			path = "tower_icon2.png";
		mi = MiniIcon::create(path, false);
		mi->_delegate = miniLayer;
		miniLayer->addChild(mi);
		mi->updatePosition(tower->getSpawnPoint());
		mi->setCharId(tower->getCharId());

		_towerIconArray.push_back(mi);
	}

	for (auto player : getGameLayer()->_CharacterArray)
	{
		if (player->isPlayerOrCom())
		{
			MiniIcon *mi;
			const char *path = "";
			if (player->isPlayer())
			{
				path = "player_icon.png";
			}
			else if (player->isCom())
			{
				if (player->isGuardian())
				{
					path = "guardian_icon.png";
				}
				else
				{
					if (player->getGroup() == currentPlayer->getGroup())
					{
						path = "com_icon.png";
					}
					else
					{
						path = "enemy_icon.png";
					}
				}
			}

			mi = MiniIcon::create(path, true);
			mi->_delegate = miniLayer;
			miniLayer->addChild(mi);
			mi->updatePosition(player->getSpawnPoint());
			mi->setCharId(player->getCharId());
		}
	}

	addChild(miniLayer, 500);

	// Call after all addChild functions
	// thats can make ActionButton::setLock -> setMask work
	updateSpecialSkillButtons();

	_isInitPlayerHud = true;
}

void HudLayer::addMapIcon()
{
	for (auto player : getGameLayer()->_CharacterArray)
	{
		if (player->isGuardian())
		{
			MiniIcon *mi;
			const char *path = "guardian_icon.png";
			mi = MiniIcon::create(path, true);
			mi->_delegate = miniLayer;
			miniLayer->addChild(mi);
			mi->updatePosition(player->getSpawnPoint());
			mi->setCharId(player->getCharId());
		}
	}
}

void HudLayer::setOnlinePingMs(int ms)
{
	if (!pingLabel)
		return;
	pingLabel->setString(format("{} ms", ms).c_str());
}

void HudLayer::tickOnlinePingLatency(float dt)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	(void)dt;
	if (hudOnlineWsConnected())
		NetLatencyPingSend();
#else
	(void)dt;
#endif
}

void HudLayer::updateGears()
{
	if (!gear1Button && !gear2Button && !gear3Button)
	{
		gear1Button = ActionButton::create("gearbg.png");
		gear1Button->setPosition(Vec2(0, winSize.height - 92));
		gear1Button->setDelegate(this);
		gear1Button->setCD(15000);
		gear1Button->setABType(GearBtn);

		gear1Button->setMarkSprite("gear_freeze.png");
		gear1Button->_gearType = GearType::None;
		addChild(gear1Button, 200);

		gear2Button = ActionButton::create("gearbg.png");
		gear2Button->setPosition(Vec2(35, winSize.height - 112));
		gear2Button->setDelegate(this);
		gear2Button->setCD(15000);
		gear2Button->_gearType = GearType::None;
		gear2Button->setABType(GearBtn);
		gear2Button->setMarkSprite("gear_freeze.png");

		addChild(gear2Button, 200);

		gear3Button = ActionButton::create("gearbg.png");
		gear3Button->setPosition(Vec2(70, winSize.height - 92));
		gear3Button->setCD(15000);
		gear3Button->setDelegate(this);
		gear3Button->_gearType = GearType::None;

		gear3Button->setABType(GearBtn);
		gear3Button->setMarkSprite("gear_freeze.png");
		addChild(gear3Button, 200);

		return;
	}

	int i = 0;
	for (auto gear : getGameLayer()->currentPlayer->getGearArray())
	{
		if (gear != gear1Button->_gearType && i == 0)
		{
			gear1Button->setGearType(gear);
		}
		else if (gear != gear2Button->_gearType && i == 1)
		{
			gear2Button->setGearType(gear);
		}
		else if (gear != gear3Button->_gearType && i == 2)
		{
			gear3Button->setGearType(gear);
		}

		if (gear == GearType::Gear06 && getItem2Button()->_gearType == GearType::None)
		{
			getItem2Button()->_gearType = gear;
			getItem2Button()->setVisible(true);
			getItem2Button()->_isLock = false;
		}
		else if (gear == GearType::Gear00 && getItem3Button()->_gearType == GearType::None)
		{
			getItem3Button()->_gearType = gear;
			getItem3Button()->_isLock = false;
			getItem3Button()->setVisible(true);
		}
		else if (gear == GearType::Gear03 && getItem4Button()->_gearType == GearType::None)
		{
			getItem4Button()->setVisible(true);
			getItem4Button()->_gearType = gear;
			getItem4Button()->_isLock = false;
		}

		i++;
	}
}

void HudLayer::setHPLose(float percent)
{
	RotateTo *ra = RotateTo::create(0.2f, -((1 - percent) * 180), -((1 - percent) * 180));
	status_hpbar->runAction(ra);

	uint32_t hp = getGameLayer()->currentPlayer->getHP();
	hpLabel->setString(to_cstr(hp));
}

void HudLayer::setCKRLose(bool isCRK2)
{
	if (!isCRK2)
	{
		if (skill4Button)
		{
			if (skill4Button->lockLabel1)
			{
				skill4Button->ougismarkSprite->setVisible(false);
				skill4Button->_isMarkVisable = false;
				auto su = ScaleBy::create(0.2f, 1.1f);
				auto call = CallFunc::create(std::bind(&ActionButton::setProgressMark, skill4Button));
				auto seq = newSequence(su, su->reverse(), call);
				skill4Button->runAction(seq);
			}
			else
			{
				skill4Button->updateProgressMark();
			}
		}
	}
	else
	{
		if (skill5Button)
		{
			if (skill5Button->lockLabel1)
			{
				skill5Button->ougismarkSprite->setVisible(false);
				skill5Button->_isMarkVisable = false;
				auto su = ScaleBy::create(0.2f, 1.1f);
				auto call = CallFunc::create(std::bind(&ActionButton::setProgressMark, skill5Button));
				auto seq = newSequence(su, su->reverse(), call);
				skill5Button->runAction(seq);
			}
			else
			{
				skill5Button->updateProgressMark();
			}
		}
	}
}

void HudLayer::setEXPLose()
{
	int exp = getGameLayer()->currentPlayer->getEXP();
	int lvExp = (getGameLayer()->currentPlayer->getLV() - 1) * 500;
	float Percent = (exp - lvExp) / 500.0f * 100;
	if (Percent > 100)
		Percent = 100;

	// RotateTo* ra=RotateTo::create(0.2f,((1-Percent/100)*90),((1-Percent/100)*90));
	// status_expbar->runAction(ra);
	status_expbar->setPercentage((1 + Percent / 100) * 50);

	if (exp >= 2500)
	{
		status_expbar->setPercentage(100);
		expLabel->setString("Max");
	}
	else
	{
		expLabel->setString(format("{}%", (int)Percent).c_str());
	}
}

void HudLayer::setTowerState(int charId)
{
	for (auto mi : _towerIconArray)
	{
		if (mi->getCharId() == charId)
		{
			mi->updateState();
		}
	}
}

void HudLayer::setCoin(const char *value)
{
	auto *player = getGameLayer()->currentPlayer;
	if (!coinLabel || !player)
	{
		return;
	}
	const int next = static_cast<int>(player->getCoin()) + to_int(value);
	coinLabel->setString(formatCoinForDisplay(next).c_str());
}

bool HudLayer::offCoin(const char *value)
{
	auto *player = getGameLayer()->currentPlayer;
	if (!coinLabel || !player)
	{
		return false;
	}
	const int sub = to_int(value);
	const int tempCoin = static_cast<int>(player->getCoin());
	if (tempCoin - sub >= 0)
	{
		player->minusCoin(static_cast<uint32_t>(sub));
		coinLabel->setString(formatCoinForDisplay(static_cast<int>(player->getCoin())).c_str());
		return true;
	}
	return false;
}

void HudLayer::setReport(const string &slayer, const string &dead, uint32_t killNum)
{
	bool isDisplay = _reportListArray.empty();

	if (isDisplay)
	{
		float length;
		reportSprite = createReport(slayer, dead, length);
		reportSprite->setPosition(Vec2(winSize.width / 2 - length / 2, winSize.height - 80));
		addChild(reportSprite, 500);

		auto su = ScaleBy::create(0.2f, 1.0f);
		auto delay = DelayTime::create(2.0f);
		auto call = CallFunc::create(std::bind(&HudLayer::setReportCache, this));
		auto seq = newSequence(su, su->reverse(), delay, call);
		reportSprite->runAction(seq);
	}

	_reportListArray.push_back({
		.slayer = slayer,
		// .char1Id = charId,
		.dead = dead,
		.num = 1,
		.kills = killNum,
		.isDisplay = isDisplay,
	});
}

void HudLayer::setReportCache()
{
	reportSprite->removeFromParent();

	for (auto &data : _reportListArray)
	{
		if (data.isDisplay) // TODO: Improve performance
			continue;

		float length;
		reportSprite = createReport(data.slayer, data.dead, length);
		reportSprite->setPosition(Vec2(winSize.width / 2 - length / 2, winSize.height - 80));
		addChild(reportSprite, 500);

		auto su = ScaleBy::create(0.2f, 1.1f);
		auto delay = DelayTime::create(2.0f);
		auto call = CallFunc::create(std::bind(&HudLayer::setReportCache, this));
		auto seq = newSequence(su, su->reverse(), delay, call);
		reportSprite->runAction(seq);

		if (data.dead != kRoleTower)
		{
			int num2 = 0;
			for (auto &data2 : _reportListArray)
			{
				if (data2.slayer == data.slayer &&
					data2.dead != kRoleTower &&
					// data2.charId == data.charId &&
					data2.isDisplay)
				{
					num2 += 1;
				}
			}

			if (num2 == 1)
			{
				if (reportSPCSprite)
				{
					reportSPCSprite->stopAllActions();
					reportSPCSprite->removeFromParent();
				}
				reportSPCSprite = createSPCReport(data.kills, num2);
				reportSPCSprite->setPosition(Vec2(winSize.width / 2, winSize.height - 115));
				addChild(reportSPCSprite, 500);
				auto fn = FadeIn::create(0.8f);
				auto mv = MoveBy::create(0.8f, Vec2(0, 10));
				auto sp = Spawn::createWithTwoActions(fn, mv);
				auto delay2 = DelayTime::create(1.6f);
				auto call2 = CallFunc::create(std::bind(&HudLayer::clearSPCReport, this));
				auto seq2 = newSequence(sp, delay2, call2);
				reportSPCSprite->runAction(seq2);
			}
			else if (num2 >= 2)
			{
				if (reportSPCSprite)
				{
					reportSPCSprite->stopAllActions();
					reportSPCSprite->removeFromParent();
				}
				reportSPCSprite = createSPCReport(data.kills, num2);
				reportSPCSprite->setScale(2.0);
				reportSPCSprite->setPosition(Vec2(winSize.width / 2, winSize.height - 105));
				addChild(reportSPCSprite, 500);
				auto st = ScaleTo::create(0.8f, 1.0f, 1.0f);
				auto call2 = CallFunc::create(std::bind(&HudLayer::stopSPCReport, this));
				auto seq2 = newSequence(st, call2);
				reportSPCSprite->runAction(seq2);

				auto sk = RepeatForever::create(CCShake::create(0.2f, 4));
				reportSPCSprite->runAction(sk);

				schedule(schedule_selector(HudLayer::updateSPCReprot), 0.2f);
			}
		}

		data.isDisplay = true;
		return;
	}

	_reportListArray.clear();
}

void HudLayer::setBuffDisplay(const char *buffName, float buffStayTime)
{
	// TODO: Display buff sprite
	return;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	float scale = 0.35f;
#else
	float scale = 0.40f;
#endif

	auto buffSprite = Sprite::createWithSpriteFrameName(format("{}_{}.png", getGameLayer()->currentPlayer->getName(), buffName).c_str());
	if (buffSprite == nullptr)
		return;
	buffSprite->setAnchorPoint(Vec2(0, 0));
	buffSprite->setPosition(Vec2(8, item1Button->getPositionY() + 76 - 28 * _buffCount));
	if (buffStayTime != 0)
	{
		if (_buffCount == 0)
		{
			bcdLabel1 = CCLabelBMFont::create(to_cstr(buffStayTime), Fonts::Default);
			bcdLabel1->setScale(scale);
			bcdLabel1->setPosition(Vec2(5, 5));
			bcdLabel1->setAnchorPoint(Vec2(0.5, 0.5));
			buffSprite->addChild(bcdLabel1);
			schedule(schedule_selector(HudLayer::updateBuffDisplay), 1.0f);
		}
		else
		{
			bcdLabel2 = CCLabelBMFont::create(to_cstr(buffStayTime), Fonts::Default);
			bcdLabel2->setScale(scale);
			bcdLabel2->setPosition(Vec2(5, 5));
			bcdLabel2->setAnchorPoint(Vec2(0.5, 0.5));
			buffSprite->addChild(bcdLabel2);
			schedule(schedule_selector(HudLayer::updateBuffDisplay2), 1.0f);
		}
	}

	auto delay = DelayTime::create(buffStayTime - 3);
	auto call = CallFunc::create(std::bind(&HudLayer::clearBuffDisplay, this, buffSprite));
	auto seq = newSequence(delay, call);
	buffSprite->runAction(seq);
	buffSprite->setTag(0);

	addChild(buffSprite, 500);
	_buffCount++;
}

void HudLayer::updateBuffDisplay(float dt)
{
	if (bcdLabel1)
	{
		int bcdValue1 = to_int(bcdLabel1->getString());
		if (bcdValue1 > 0)
		{
			bcdLabel1->setString(to_cstr(bcdValue1 - 1));
		}
		else
		{
			bcdLabel1->removeFromParent();
			bcdLabel1 = nullptr;
			_buffCount--;
		}
	}
}

void HudLayer::updateBuffDisplay2(float dt)
{
	if (bcdLabel2)
	{
		int bcdValue2 = to_int(bcdLabel2->getString());
		if (bcdValue2 > 0)
		{
			bcdLabel2->setString(to_cstr(bcdValue2 - 1));
		}
		else
		{
			bcdLabel2->removeFromParent();
			bcdLabel2 = nullptr;
			_buffCount--;
		}
	}
}

void HudLayer::clearBuffDisplay(Sprite *bs)
{
	if (bs->getTag() == 0)
	{
		auto call = CallFunc::create(std::bind(&HudLayer::clearBuffDisplay, this, bs));
		auto delay = DelayTime::create(4.0f);
		bs->setTag(1);
		auto fade = FadeOut::create(0.5f);
		auto fadeseq = RepeatForever::create(newSequence(fade, fade->reverse()));
		auto seq = newSequence(delay, call);
		bs->runAction(seq);
		bs->runAction(fadeseq);
	}
	else
	{
		bs->removeFromParent();
	}
}

void HudLayer::clearSPCReport()
{
	if (reportSPCSprite)
	{
		reportSPCSprite->stopAllActions();
		reportSPCSprite->removeFromParent();
		reportSPCSprite = nullptr;
	}
}

void HudLayer::stopSPCReport()
{
	if (reportSPCSprite)
	{
		reportSPCSprite->stopAllActions();
		unschedule(schedule_selector(HudLayer::updateSPCReprot));
		reportSPCSprite->setPosition(Vec2(winSize.width / 2, winSize.height - 105));
		reportSPCSprite->setScale(1.0f);
		auto delay = DelayTime::create(1.6f);
		auto call = CallFunc::create(std::bind(&HudLayer::clearSPCReport, this));
		reportSPCSprite->runAction(newSequence(delay, call));
	}
}

void HudLayer::updateSPCReprot(float dt)
{
	if (reportSPCSprite)
	{
		reportSPCSprite->setPosition(Vec2(winSize.width / 2, winSize.height - 105));
	}
}

Sprite *HudLayer::createSPCReport(uint32_t killNum, int num)
{
	bool isBrocast = false;
	if (UserDefault::sharedUserDefault()->getBoolForKey("isVoice"))
	{
		isBrocast = true;
	}

	if (num == 1)
	{
		if (killNum < 10)
		{
			reportSPCSprite = Sprite::createWithSpriteFrameName("DoubleKill_rpf.png");
			if (isBrocast)
				SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/kill2_1.wav");
		}
		else if (killNum < 20)
		{
			reportSPCSprite = Sprite::createWithSpriteFrameName("KillingSpree_rpf.png");
			if (isBrocast)
				SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/kill2_2.wav");
		}
		else if (killNum < 30)
		{
			reportSPCSprite = Sprite::createWithSpriteFrameName("Unstoppable_rpf.png");
			if (isBrocast)
				SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/kill2_3.wav");
		}
		else
		{
			reportSPCSprite = Sprite::createWithSpriteFrameName("Godlike_rpf.png");
			if (isBrocast)
				SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/kill2_4.wav");
		}
	}
	else if (num == 2)
	{
		if (killNum < 10)
		{
			reportSPCSprite = Sprite::createWithSpriteFrameName("TripeKill_rpf.png");
			if (isBrocast)
				SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/kill3_1.wav");
		}
		else if (killNum < 20)
		{
			reportSPCSprite = Sprite::createWithSpriteFrameName("Domination_rpf.png");
			if (isBrocast)
				SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/kill3_2.wav");
		}
		else if (killNum < 30)
		{
			reportSPCSprite = Sprite::createWithSpriteFrameName("Rampage_rpf.png");
			if (isBrocast)
				SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/kill3_3.wav");
		}
		else if (killNum >= 30)
		{
			reportSPCSprite = Sprite::createWithSpriteFrameName("Holyshit_rpf.png");
			if (isBrocast)
				SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/kill3_4.wav");
		}
	}
	else
	{
		reportSPCSprite = Sprite::createWithSpriteFrameName("MonsterKill_rpf.png");
		if (isBrocast)
			SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/kill4.wav");
	}

	return reportSPCSprite;
}

Sprite *HudLayer::createReport(const string &slayer, const string &dead, float &length)
{
	Sprite *reportSprite = Sprite::create();

	Sprite *slain_p = Sprite::createWithSpriteFrameName(format("{}_rp.png", slayer).c_str());
	slain_p->setAnchorPoint(Vec2(0, 0.5f));
	slain_p->setPosition(Vec2(0, 0));
	reportSprite->addChild(slain_p);

	Sprite *slain_pf = Sprite::createWithSpriteFrameName(format("{}_rpf.png", slayer).c_str());
	slain_pf->setPosition(Vec2(slain_p->getContentSize().width, 0));
	slain_pf->setAnchorPoint(Vec2(0, 0.5f));
	reportSprite->addChild(slain_pf);

	Sprite *slain;
	if (dead != kRoleTower)
	{
		slain = Sprite::createWithSpriteFrameName("slain.png");
		reportSprite->addChild(slain);
	}
	else
	{
		slain = Sprite::createWithSpriteFrameName("destroy.png");
		reportSprite->addChild(slain);
	}
	slain->setAnchorPoint(Vec2(0, 0.5f));
	slain->setPosition(Vec2(slain_pf->getPositionX() + slain_pf->getContentSize().width, 0));

	Sprite *death_pf = Sprite::createWithSpriteFrameName(format("{}_rpf.png", dead).c_str());
	death_pf->setAnchorPoint(Vec2(0, 0.5f));
	death_pf->setPosition(Vec2(slain->getPositionX() + slain->getContentSize().width, 0));
	reportSprite->addChild(death_pf);
	Sprite *death_p;
	if (dead != kRoleTower)
	{
		death_p = Sprite::createWithSpriteFrameName(format("{}_rp.png", dead).c_str());
		death_p->setAnchorPoint(Vec2(0, 0.5f));
		death_p->setPosition(Vec2(death_pf->getPositionX() + death_pf->getContentSize().width, 0));
		reportSprite->addChild(death_p);
	}

	if (dead != kRoleTower)
	{
		length = slain_p->getContentSize().width + slain_pf->getContentSize().width + slain->getContentSize().width + death_pf->getContentSize().width + death_p->getContentSize().width;
	}
	else
	{
		length = slain_p->getContentSize().width + slain_pf->getContentSize().width + slain->getContentSize().width + death_pf->getContentSize().width;
	}

	return reportSprite;
}

void HudLayer::attackButtonClick(ABType type)
{
	getGameLayer()->attackButtonClick(type);
}

void HudLayer::gearButtonClick(GearType type)
{
	getGameLayer()->gearButtonClick(type);
}

void HudLayer::attackButtonRelease()
{
	getGameLayer()->attackButtonRelease();
}

void HudLayer::pauseButtonClick(Ref *sender)
{
	getGameLayer()->onPause();
}

void HudLayer::gearButtonClick(Ref *sender)
{
	if (!_isAllButtonLocked)
	{
		getGameLayer()->onGear();
	}
}

bool HudLayer::getSkillFinish()
{
	return getGameLayer()->getSkillFinish();
}

bool HudLayer::getOugisEnable(bool isCKR2)
{
	if (!isCKR2)
	{
		uint32_t ckr = getGameLayer()->currentPlayer->getCKR();
		return ckr >= 15000;
	}
	else
	{
		uint32_t ckr2 = getGameLayer()->currentPlayer->getCKR2();
		return ckr2 >= 25000;
	}
}

void HudLayer::costCKR(uint32_t value, bool isCKR2)
{
	if (!isCKR2)
	{
		uint32_t ckr = getGameLayer()->currentPlayer->getCKR();
		ckr = ckr >= value ? ckr - value : 0;

		getGameLayer()->currentPlayer->setCKR(ckr);
		setCKRLose(false);
	}
	else
	{
		uint32_t ckr2 = getGameLayer()->currentPlayer->getCKR2();
		ckr2 = ckr2 >= value ? ckr2 - value : 0;

		getGameLayer()->currentPlayer->setCKR2(ckr2);
		setCKRLose(true);
	}
}

void HudLayer::setOugis(const string &name, Group group)
{
	if (!ougisLayer)
	{
		Ref *pObject;
		CCArray *childArray = getChildren();
		CCARRAY_FOREACH(childArray, pObject)
		{
			Node *object = (Node *)pObject;
			object->pauseSchedulerAndActions();
		}

		ougisLayer = Layer::create();
		int startPosY = 0;
		int startPosX = 0;
		int endPosX = 0;
		Sprite *CutBg;
		const char *cutPath1;
		const char *cutPath2;
		if (getGameLayer()->playerGroup == Group::Konoha)
		{
			cutPath1 = "CutBg.png";
			cutPath2 = "CutBg2.png";
		}
		else
		{
			cutPath1 = "CutBg2.png";
			cutPath2 = "CutBg.png";
		}

		bool isKonohaGroup = group == Group::Konoha;
		if (isKonohaGroup)
		{
			startPosY = 0;
			startPosX = -48;
			endPosX = 0;
			CutBg = Sprite::createWithSpriteFrameName(cutPath1);
			CutBg->setAnchorPoint(Vec2(0, 0));
		}
		else
		{
			startPosY = winSize.height / 2;
			startPosX = winSize.width + 48;
			endPosX = winSize.width;
			CutBg = Sprite::createWithSpriteFrameName(cutPath2);
			CutBg->setFlipX(true);
			CutBg->setAnchorPoint(Vec2(1, 0.5f));
		}

		CutBg->setPosition(Vec2(startPosX, startPosY));
		CutBg->runAction(MoveTo::create(0.3f, Vec2(endPosX, startPosY)));
		ougisLayer->addChild(CutBg);

		Vector<SpriteFrame *> spriteFrames;
		for (int i = 1; i <= 8; i++)
		{
			auto frame = getSpriteFrame("CutLine_{:02d}.png", i);
			spriteFrames.pushBack(frame);
		}

		auto tempAnimation = Animation::createWithSpriteFrames(spriteFrames, 0.1f);
		auto tempAction = Animate::create(tempAnimation);

		Sprite *CutLine = Sprite::createWithSpriteFrameName("CutLine_01.png");
		if (isKonohaGroup)
		{
			CutLine->setAnchorPoint(Vec2(0, 0));
			CutLine->setPosition(Vec2(-48, 0));
			CutLine->runAction(tempAction);
			CutLine->runAction(MoveTo::create(0.3f, Vec2(0, 0)));
		}
		else
		{
			CutLine->setFlipX(true);
			CutLine->setAnchorPoint(Vec2(1, 0.5f));
			CutLine->setPosition(Vec2(winSize.width + 48, winSize.height / 2));
			CutLine->runAction(tempAction);
			CutLine->runAction(MoveTo::create(0.3f, Vec2(winSize.width, winSize.height / 2)));
		}

		ougisLayer->addChild(CutLine);

		Sprite *CutLineUP = Sprite::createWithSpriteFrameName("CutLineUP.png");
		if (isKonohaGroup)
		{
			CutLineUP->setAnchorPoint(Vec2(0, 0));
			CutLineUP->setPosition(Vec2(-48, 2));
			CutLineUP->runAction(MoveTo::create(0.3f, Vec2(-1, 1)));
		}
		else
		{
			CutLineUP->setFlipX(true);
			CutLineUP->setAnchorPoint(Vec2(1, 0.5f));
			CutLineUP->setPosition(Vec2(winSize.width + 48, winSize.height / 2 + 2));
			CutLineUP->runAction(MoveTo::create(0.3f, Vec2(winSize.width + 1, winSize.height / 2 + 1)));
		}

		ougisLayer->addChild(CutLineUP);

		Sprite *CutIn = Sprite::createWithSpriteFrameName(format("{}_CutIn.png", name).c_str());
		if (isKonohaGroup)
		{
			CutIn->setAnchorPoint(Vec2(0, 0));
			CutIn->setPosition(Vec2(-200, 10));
			CutIn->runAction(MoveTo::create(0.3f, Vec2(0, 10)));
		}
		else
		{
			CutIn->setAnchorPoint(Vec2(1, 0));
			CutIn->setPosition(Vec2(winSize.width + 200, winSize.height / 2 + 10 - CutBg->getContentSize().height / 2));
			CutIn->runAction(MoveTo::create(0.3f, Vec2(winSize.width, winSize.height / 2 + 10 - CutBg->getContentSize().height / 2)));
		}
		ougisLayer->addChild(CutIn);

		Sprite *CutLineDown = Sprite::createWithSpriteFrameName("CutLineDown.png");
		if (isKonohaGroup)
		{
			CutLineDown->setAnchorPoint(Vec2(0, 0));
			CutLineDown->setPosition(Vec2(-48, 0));
			CutLineDown->runAction(MoveTo::create(0.3f, Vec2(0, 0)));
		}
		else
		{
			CutLineDown->setFlipX(true);
			CutLineDown->setAnchorPoint(Vec2(1, 0));
			CutLineDown->setPosition(Vec2(winSize.width + 48, winSize.height / 2 - CutBg->getContentSize().height / 2));
			CutLineDown->runAction(MoveTo::create(0.3f, Vec2(winSize.width, winSize.height / 2 - CutBg->getContentSize().height / 2)));
		}

		ougisLayer->addChild(CutLineDown);

		ougisLayer->setPosition(Vec2(0, 0));
		addChild(ougisLayer, -100);

		auto delay = DelayTime::create(1.0f);
		auto call = CallFunc::create(std::bind(&HudLayer::removeOugis, this));
		ougisLayer->runAction(newSequence(delay, call));
	}
}

void HudLayer::removeOugis()
{
	ougisLayer->removeFromParent();
	ougisLayer = nullptr;

	Ref *pObject;
	CCArray *childArray = getChildren();
	CCARRAY_FOREACH(childArray, pObject)
	{
		Node *object = (Node *)pObject;
		object->resumeSchedulerAndActions();
	}

	getGameLayer()->removeOugis();
}

void HudLayer::initSkillButtons()
{
	// TODO: For Ino ougi2 (Player can control other character's skills)
}

void HudLayer::setSkillButtons(bool isVisable)
{
	// skills
	skill1Button->setVisible(isVisable);
	skill2Button->setVisible(isVisable);
	skill3Button->setVisible(isVisable);
	skill4Button->setVisible(isVisable);
	skill5Button->setVisible(isVisable);
	// items
	item1Button->setVisible(isVisable);
	item2Button->setVisible(isVisable);
	item3Button->setVisible(isVisable);
	item4Button->setVisible(isVisable);
}

void HudLayer::updateSkillButtons()
{
	auto player = getGameLayer()->currentPlayer;
	if (!player)
		return;
	auto charName = player->getName();
	if (charName.length() == 0)
		return;

	// update avator
	auto avator = Sprite::createWithSpriteFrameName((charName + "_avator.png").c_str());
	if (avator)
		gearMenuSprite->setNormalImage(avator);

	auto cache = SpriteFrameCache::sharedSpriteFrameCache();
	SpriteFrame *frame;

#define updateButtonInfo(index)                                                    \
	frame = cache->spriteFrameByName((charName + "_skill" #index ".png").c_str()); \
	if (skill##index##Button)                                                      \
	{                                                                              \
		if (frame != nullptr)                                                      \
			skill##index##Button->setDisplayFrame(frame);                          \
		skill##index##Button->setCD(player->_sAttackCD##index * 1000);             \
		skill##index##Button->_isColdChanged = true;                               \
	}

#define updateOugiButtonInfo(index)                                                \
	frame = cache->spriteFrameByName((charName + "_skill" #index ".png").c_str()); \
	if (skill##index##Button)                                                      \
	{                                                                              \
		if (frame != nullptr)                                                      \
			skill##index##Button->setDisplayFrame(frame);                          \
		skill##index##Button->_isColdChanged = true;                               \
	}

	updateButtonInfo(1);
	updateButtonInfo(2);
	updateButtonInfo(3);
	updateOugiButtonInfo(4);
	updateOugiButtonInfo(5);
	updateSpecialSkillButtons();
}

void HudLayer::updateSpecialSkillButtons()
{
	auto currentPlayer = getGameLayer()->currentPlayer;
	if (currentPlayer == nullptr)
	{
		CCLOGERROR("Current player is null.");
		return;
	}

	skill1Button->setDoubleSkill(currentPlayer->_sAttack1isDouble);
	skill2Button->setDoubleSkill(currentPlayer->_sAttack2isDouble);
	skill3Button->setDoubleSkill(currentPlayer->_sAttack3isDouble);
	skill1Button->setCD(currentPlayer->_sAttackCD1 * 1000);
	skill2Button->setCD(currentPlayer->_sAttackCD2 * 1000);
	skill3Button->setCD(currentPlayer->_sAttackCD3 * 1000);
	skill1Button->_isColdChanged = true;
	skill2Button->_isColdChanged = true;
	skill3Button->_isColdChanged = true;
	if (skill1Button->_isLock)
		skill1Button->unLock();
	if (skill2Button->_isLock)
		skill2Button->unLock();
	if (skill3Button->_isLock)
		skill3Button->unLock();
}

void HudLayer::resetSkillButtons()
{
	skill1Button->reset();
	skill2Button->reset();
	skill3Button->reset();
	skill4Button->reset();
	skill5Button->reset();
}
