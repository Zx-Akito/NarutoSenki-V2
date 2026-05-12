#include <cctype>
#include <cstdio>

#include "StartMenu.h"
#include "Constants/GameAuthConfig.h"
#include "Constants/UiFlowKeys.hpp"
#include "GameLoginHttp.h"
#include "MyUtils/KTools.h"
#include "GUI/CCControlExtension/CCScale9Sprite.h"

USING_NS_CC_EXT;

GameMode s_GameMode = GameMode::Classic;
std::array<std::unique_ptr<IGameModeHandler>, GameMode::__Internal_Max_Length> s_ModeHandlers = {
	std::make_unique<Mode1v1>(),
	std::make_unique<Mode3v3>(),
	std::make_unique<Mode4v4>(),
	std::make_unique<ModeHardCore>(),
	std::make_unique<ModeBoss>(),
	std::make_unique<ModeClone>(false),
	std::make_unique<ModeDeathmatch>(),
	std::make_unique<ModeRandomDeathmatch>(),
};
int Cheats = 0;
bool enableCustomSelect = false;

/*----------------------
init MenuButton ;
----------------------*/

bool MenuButton::init(const char *szImage)
{
	RETURN_FALSE_IF(!Sprite::init());

	initWithSpriteFrameName(szImage);
	setAnchorPoint(Vec2(0.5, 0));

	return true;
}

void MenuButton::onEnter()
{
	Sprite::onEnter();
	Director::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, 10, true);

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

	// JniMethodInfo minfo;

	// bool isHave = JniHelper::getStaticMethodInfo(minfo, "net/zakume/game/NarutoSenki", "showBanner", "()V");
	// if (isHave)
	// {
	// 	minfo.env->CallStaticVoidMethod(minfo.classID, minfo.methodID);
	// }

#endif
}

void MenuButton::onExit()
{
	Sprite::onExit();
	Director::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
}

CCRect MenuButton::getRect()
{
	CCSize size = getContentSize();
	return CCRect(0, 0, size.width, size.height);
}

bool MenuButton::containsTouchLocation(Touch *touch)
{
	return getRect().containsPoint(convertTouchToNodeSpace(touch));
}

void MenuButton::setBtnType(MenuButtonType type)
{
	_type = type;
}

MenuButtonType MenuButton::getBtnType()
{
	return _type;
}

bool MenuButton::ccTouchBegan(Touch *touch, Event *event)
{
	// touch area
	if (!containsTouchLocation(touch))
		return false;

	// click();

	prePosY = 0;

	return true;
}

void MenuButton::ccTouchMoved(Touch *touch, Event *event)
{
	// touch area
	if (prePosY == 0)
	{
		prePosY = touch->getLocation().y;
	}
	else
	{
		if (getBtnType() != MenuButtonType::HardCore)
		{
			if (abs(touch->getLocation().y - prePosY) > 16)
			{
				if (touch->getLocation().y < prePosY)
				{
					_startMenu->isClockwise = true;
				}
				else
				{
					_startMenu->isClockwise = false;
				}
				_startMenu->isDrag = true;
			}
		}
	}
}

void MenuButton::ccTouchEnded(Touch *touch, Event *event)
{
	if (_isTop && !_startMenu->isDrag)
	{
		switch (_type)
		{
		case MenuButtonType::Training:
			SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.wav");
			_startMenu->onTrainingCallBack();
			break;
		case MenuButtonType::Credits:
			SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.wav");
			_startMenu->onCreditsCallBack();
			break;
		case MenuButtonType::Exit:
			SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.wav");
			_startMenu->onExitCallBack();
			break;
		case MenuButtonType::Custom:
			SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.wav");
			_startMenu->onCustomCallBack();
			break;
		case MenuButtonType::HardCore:
			SimpleAudioEngine::sharedEngine()->playEffect(SELECT_SOUND);
			auto frame = getSpriteFrame("menu05_text.png");
			_startMenu->menuText->setDisplayFrame(frame);
			_startMenu->onHardLayerCallBack();
			break;
		}
	}
	else
	{
		SimpleAudioEngine::sharedEngine()->playEffect(SELECT_SOUND);
		prePosY = 0;
		_startMenu->scrollMenu(getPositionY());
		_startMenu->isDrag = false;
	}
}

void MenuButton::playSound()
{
	SimpleAudioEngine::sharedEngine()->stopAllEffects();

	switch (_type)
	{
	case MenuButtonType::Training:
		SimpleAudioEngine::sharedEngine()->playEffect(TRAINING_SOUND);
		break;
	case MenuButtonType::Custom:
		SimpleAudioEngine::sharedEngine()->playEffect(NETWORK_SOUND);
		break;
	case MenuButtonType::Credits:
		SimpleAudioEngine::sharedEngine()->playEffect(CREDITS_SOUND);
		break;
	case MenuButtonType::Exit:
		SimpleAudioEngine::sharedEngine()->playEffect(EXIT_SOUND);
		break;
	default:
		break;
	}
}

MenuButton *MenuButton::create(const char *szImage)
{
	auto mb = new MenuButton();
	if (mb && mb->init(szImage))
	{
		mb->autorelease();
		return mb;
	}
	else
	{
		delete mb;
		return nullptr;
	}
}

/*----------------------
init StartMenu layer;
----------------------*/

StartMenu::StartMenu()
{
	isClockwise = false;
	isDrag = false;
	menuText = nullptr;
	hardCoreLayer = nullptr;
	notice_layer = nullptr;
	noticeLabel = nullptr;
	login_btn = nullptr;
	loginAccountMenu = nullptr;
	loginLayer = nullptr;
	profileLayer = nullptr;
	loginUserEdit = nullptr;
	loginPwEdit = nullptr;
	loginUserFieldBg = nullptr;
	loginPwFieldBg = nullptr;
}

bool StartMenu::init()
{
	RETURN_FALSE_IF(!Layer::init());

	addSprites("Menu.plist");
	addSprites("Result.plist");
	addSprites("NamePlate.plist");

	// Vec2 origin = Director::sharedDirector()->getVisibleOrigin();

	// Sprite* bgSprite = Sprite::create("red_bg.png");
	////pSprite->setPosition(Vec2(winSize.width/2 + origin.x, winSize.height/2 + origin.y));
	// FULL_SCREEN_SPRITE(bgSprite);
	// bgSprite->setAnchorPoint(Vec2(0,0));
	// bgSprite->setPosition(Vec2(0,0));
	// addChild(bgSprite, -5);

	// produce groud
	Sprite *gold_left = Sprite::createWithSpriteFrameName("gold_left.png");
	gold_left->setAnchorPoint(Vec2(0, 0));
	gold_left->setPosition(Vec2(0, 20));
	addChild(gold_left, 1);

	Sprite *gold_right = Sprite::createWithSpriteFrameName("gold_right.png");
	gold_right->setAnchorPoint(Vec2(0, 1));
	gold_right->setPosition(Vec2(winSize.width - gold_right->getContentSize().width - 20, winSize.height - 20));
	addChild(gold_right, 1);

	// produce the cloud
	Sprite *cloud_left = Sprite::createWithSpriteFrameName("cloud.png");
	cloud_left->setPosition(Vec2(0, 15));
	cloud_left->setFlipX(true);
	cloud_left->setFlipY(true);
	cloud_left->setAnchorPoint(Vec2(0, 0));
	addChild(cloud_left, 1);

	auto cmv1 = MoveBy::create(1, Vec2(-15, 0));
	auto cseq1 = RepeatForever::create(newSequence(cmv1, cmv1->reverse()));
	cloud_left->runAction(cseq1);

	Sprite *cloud_right = Sprite::createWithSpriteFrameName("cloud.png");
	cloud_right->setPosition(Vec2(winSize.width - cloud_right->getContentSize().width,
								  winSize.height - (cloud_right->getContentSize().height + 15)));
	cloud_right->setAnchorPoint(Vec2(0, 0));
	addChild(cloud_right, 1);

	auto cmv2 = MoveBy::create(1, Vec2(15, 0));
	auto cseq2 = RepeatForever::create(newSequence(cmv2, cmv2->reverse()));
	cloud_right->runAction(cseq2);

	// produce the menu_bar
	Sprite *menu_bar_b = Sprite::create("menu_bar2.png");
	menu_bar_b->setAnchorPoint(Vec2(0, 0));
	FULL_SCREEN_SPRITE(menu_bar_b);
	addChild(menu_bar_b, 2);

	Sprite *menu_bar_t = Sprite::create("menu_bar3.png");
	menu_bar_t->setAnchorPoint(Vec2(0, 0));
	menu_bar_t->setPosition(Vec2(0, winSize.height - menu_bar_t->getContentSize().height));
	FULL_SCREEN_SPRITE(menu_bar_t);
	addChild(menu_bar_t, 2);

	Sprite *startmenu_title = Sprite::createWithSpriteFrameName("startmenu_title.png");
	startmenu_title->setAnchorPoint(Vec2(0, 0));
	startmenu_title->setPosition(Vec2(2, winSize.height - startmenu_title->getContentSize().height - 2));
	addChild(startmenu_title, 3);

	// produce the menu button
	auto gamemode_btn = MenuButton::create("menu01.png");
	gamemode_btn->setDelegate(this);
	gamemode_btn->setBtnType(MenuButtonType::Custom);
	gamemode_btn->setScale(0.5f);
	gamemode_btn->setPositionY(_pos03);
	_menuArray.push_back(gamemode_btn);

	auto credits_btn = MenuButton::create("menu04.png");
	credits_btn->setDelegate(this);
	credits_btn->setBtnType(MenuButtonType::Credits);
	credits_btn->setScale(0.5f);
	credits_btn->setVisible(false);
	credits_btn->_isBottom = true;
	credits_btn->setPositionY(_pos02);
	_menuArray.push_back(credits_btn);

	auto training_btn = MenuButton::create("menu02.png");
	training_btn->setDelegate(this);
	training_btn->setBtnType(MenuButtonType::Training);
	training_btn->_isTop = true;
	training_btn->setPositionY(_pos02);
	_menuArray.push_back(training_btn);

	auto exit_btn = MenuButton::create("menu03.png");
	exit_btn->setDelegate(this);
	exit_btn->setBtnType(MenuButtonType::Exit);
	exit_btn->setScale(0.5f);
	exit_btn->setPositionY(_pos01);
	_menuArray.push_back(exit_btn);

	menuText = Sprite::createWithSpriteFrameName("menu02_text.png");
	menuText->setAnchorPoint(Vec2(0, 0));
	menuText->setPosition(Vec2(10, 2));
	addChild(menuText, 5);

	for (auto menu : _menuArray)
	{
		menu->setPositionX(105);
		addChild(menu, 2);
	}
	auto versionLabel = CCLabelBMFont::create(VERSION_CODE, Fonts::Default);
	versionLabel->setScale(0.3f);
	versionLabel->setPosition(winSize.width - 25, 10);
	addChild(versionLabel, 5);

	Sprite *avator = Sprite::createWithSpriteFrameName("avator1.png");
	avator->setAnchorPoint(Vec2(0, 0));
	avator->setOpacity(0);
	avator->setPosition(Vec2(winSize.width - avator->getContentSize().width, 19));
	addChild(avator, 1);

	Vector<SpriteFrame *> frames;
	Vector<FiniteTimeAction *> list;
	int i = 0;
	while (++i < 5)
	{
		auto frame = getSpriteFrame("avator{}.png", i);
		frames.pushBack(frame);
		auto tempAnimation = Animation::createWithSpriteFrames(frames, 0.1f);
		auto tempAction = Animate::create(tempAnimation);
		list.pushBack(tempAction);
		auto fadeIn = FadeIn::create(0.8f);
		auto delay = DelayTime::create(1.0f);
		auto fadeOut = FadeOut::create(0.5f);
		list.pushBack(fadeIn);
		list.pushBack(delay);
		list.pushBack(fadeOut);
	}

	avator->runAction(RepeatForever::create(Sequence::create(list)));
	MenuItem *news_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("news_btn.png"), nullptr, this, menu_selector(StartMenu::onNewsBtn));
	Menu *menu = Menu::create(news_btn, nullptr);
	news_btn->setAnchorPoint(Vec2(0, 0.5f));
	menu->setPosition(15, winSize.height - 50);
	addChild(menu, 5);

	setNotice();

	scheduleUpdate();

	return true;
}

void StartMenu::onEnter()
{
	Layer::onEnter();
	rebuildLoginAccountMenu();
	// SimpleAudioEngine::sharedEngine()->end();
	SimpleAudioEngine::sharedEngine()->stopBackgroundMusic(true);

	if (UserDefault::sharedUserDefault()->getBoolForKey("isBGM"))
	{
		SimpleAudioEngine::sharedEngine()->playBackgroundMusic(MENU_MUSIC, true);
	}
}

void StartMenu::onExit()
{
	Layer::onExit();
	if (profileLayer)
	{
		profileLayer->removeFromParent();
		profileLayer = nullptr;
	}
	if (loginAccountMenu)
	{
		loginAccountMenu->removeFromParent();
		loginAccountMenu = nullptr;
		login_btn = nullptr;
	}
	loginLayer = nullptr;
	loginUserEdit = nullptr;
	loginPwEdit = nullptr;
	loginUserFieldBg = nullptr;
	loginPwFieldBg = nullptr;
	// Keep audio engine alive across scene transitions (e.g. Credits),
	// otherwise newly started BGM can be cut off when StartMenu exits.
}

std::string StartMenu::trimmedUserDefaultString(const char *key)
{
	const std::string s = UserDefault::sharedUserDefault()->getStringForKey(key);
	size_t start = 0;
	while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
	{
		++start;
	}
	size_t end = s.size();
	while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
	{
		--end;
	}
	return s.substr(start, end - start);
}

void StartMenu::rebuildLoginAccountMenu()
{
	if (loginAccountMenu != nullptr)
	{
		loginAccountMenu->removeFromParent();
		loginAccountMenu = nullptr;
		login_btn = nullptr;
	}

	const std::string gameId = trimmedUserDefaultString("ns_account_game_id");
	const bool loggedIn = !gameId.empty();

	if (loggedIn)
	{
		login_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("profile_btn.png"), nullptr, this,
			menu_selector(StartMenu::onProfileBtn));
	}
	else
	{
		login_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("login_btn.png"), nullptr, this,
			menu_selector(StartMenu::onLoginBtn));
	}

	loginAccountMenu = Menu::create(login_btn, nullptr);
	login_btn->setAnchorPoint(Vec2(1, 0.5f));
	loginAccountMenu->setPosition(winSize.width - 15, winSize.height - 50);
	addChild(loginAccountMenu, 5);
}

void StartMenu::onProfileBtn(Ref *sender)
{
	if (profileLayer != nullptr || loginLayer != nullptr)
	{
		return;
	}

	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.wav");

	profileLayer = Layer::create();

	auto *bg = Sprite::createWithSpriteFrameName("profile_bg.png");
	bg->setPosition(Vec2(winSize.width / 2, winSize.height / 2));
	profileLayer->addChild(bg, 1);

	const std::string user = trimmedUserDefaultString("ns_account_username");
	const std::string id = trimmedUserDefaultString("ns_account_game_id");
	const std::string pt = trimmedUserDefaultString("ns_account_point");
	const std::string cn = trimmedUserDefaultString("ns_account_coin");
	const std::string gr = trimmedUserDefaultString("ns_account_group_id");

	char infoBuf[512];
	std::snprintf(infoBuf, sizeof(infoBuf),
		"ID: %s\nUser: %s\nPoints: %s\nCoins: %s\nGroup: %s", id.c_str(), user.c_str(), pt.c_str(), cn.c_str(),
		gr.c_str());
	auto *info = CCLabelTTF::create(infoBuf, FONT_NAME, 11);
	info->setHorizontalAlignment(kCCTextAlignmentLeft);
	info->setAnchorPoint(Vec2(0.5f, 0.5f));
	info->setPosition(Vec2(winSize.width / 2, winSize.height / 2 + 8));
	profileLayer->addChild(info, 2);

	auto *closeBtn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("profile_closeBtn.png"),
		Sprite::createWithSpriteFrameName("profile_closeBtn.png"), this, menu_selector(StartMenu::onProfileLayerClose));
	auto *logoutBtn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("yes_btn1.png"),
		Sprite::createWithSpriteFrameName("yes_btn2.png"), this, menu_selector(StartMenu::onProfileLogout));
	auto *profileMenu = Menu::create(closeBtn, logoutBtn, nullptr);
	profileMenu->setPosition(Vec2(0, 0));
	const float cx = winSize.width / 2;
	const float cy = winSize.height / 2;
	const float halfH = bg->getContentSize().height * 0.5f;
	closeBtn->setPosition(Vec2(cx - 55, cy - halfH + 28));
	logoutBtn->setPosition(Vec2(cx + 55, cy - halfH + 28));
	profileLayer->addChild(profileMenu, 3);

	addChild(profileLayer, 800);
}

void StartMenu::onProfileLayerClose(Ref *sender)
{
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/cancel.wav");
	if (!profileLayer)
	{
		return;
	}
	profileLayer->removeAllChildren();
	profileLayer->removeFromParent();
	profileLayer = nullptr;
}

void StartMenu::onProfileLogout(Ref *sender)
{
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.wav");
	UserDefault::sharedUserDefault()->setStringForKey("ns_account_username", "");
	UserDefault::sharedUserDefault()->setStringForKey("ns_account_game_id", "");
	UserDefault::sharedUserDefault()->setStringForKey("ns_account_point", "");
	UserDefault::sharedUserDefault()->setStringForKey("ns_account_coin", "");
	UserDefault::sharedUserDefault()->setStringForKey("ns_account_group_id", "");
	UserDefault::sharedUserDefault()->flush();

	if (profileLayer)
	{
		profileLayer->removeAllChildren();
		profileLayer->removeFromParent();
		profileLayer = nullptr;
	}

	rebuildLoginAccountMenu();
	addChild(CCTips::create("ProfileLoggedOut"), 5000);
}

void StartMenu::onLoginBtn(Ref *sender)
{
	if (loginLayer)
	{
		return;
	}

	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.wav");

	loginLayer = Layer::create();

	auto confirmBg = Sprite::createWithSpriteFrameName("confirm_bg.png");
	confirmBg->setPosition(Vec2(winSize.width / 2, winSize.height / 2));
	loginLayer->addChild(confirmBg, 1);

	auto userTitle = Sprite::createWithSpriteFrameName("userInput_title.png");
	userTitle->setAnchorPoint(Vec2(0, 0.5f));
	userTitle->setPosition(Vec2(winSize.width / 2 - 74, winSize.height / 2 + 28));
	loginLayer->addChild(userTitle, 2);

	loginUserFieldBg = Sprite::createWithSpriteFrameName("input_bg.png");
	loginUserFieldBg->setAnchorPoint(Vec2(0, 0.5f));
	loginUserFieldBg->setPosition(Vec2(winSize.width / 2 - 35, winSize.height / 2 + 28));
	loginLayer->addChild(loginUserFieldBg, 2);

	auto pwTitle = Sprite::createWithSpriteFrameName("pwInput_title.png");
	pwTitle->setAnchorPoint(Vec2(0, 0.5f));
	pwTitle->setPosition(Vec2(winSize.width / 2 - 74, winSize.height / 2 - 3));
	loginLayer->addChild(pwTitle, 2);

	loginPwFieldBg = Sprite::createWithSpriteFrameName("input_bg.png");
	loginPwFieldBg->setAnchorPoint(Vec2(0, 0.5f));
	loginPwFieldBg->setPosition(Vec2(winSize.width / 2 - 35, winSize.height / 2 - 3));
	loginLayer->addChild(loginPwFieldBg, 2);

	const CCSize userFieldSize = loginUserFieldBg->getContentSize();
	CCScale9Sprite *userEditBg = CCScale9Sprite::createWithSpriteFrameName("input_bg.png");
	userEditBg->setOpacity(0);
	loginUserEdit = CCEditBox::create(userFieldSize, userEditBg);
	loginUserEdit->setPosition(Vec2(
		loginUserFieldBg->getPositionX() + userFieldSize.width * 0.5f,
		loginUserFieldBg->getPositionY()));
	loginUserEdit->setFont(FONT_NAME, 12);
	loginUserEdit->setFontColor(ccc3(255, 255, 255));
	loginUserEdit->setPlaceHolder("");
	loginUserEdit->setInputMode(kEditBoxInputModeSingleLine);
	loginUserEdit->setInputFlag(kEditBoxInputFlagSensitive);
	loginUserEdit->setReturnType(kKeyboardReturnTypeDefault);
	loginUserEdit->setMaxLength(64);
	loginUserEdit->setDelegate(this);
	loginLayer->addChild(loginUserEdit, 4);

	const CCSize pwFieldSize = loginPwFieldBg->getContentSize();
	CCScale9Sprite *pwEditBg = CCScale9Sprite::createWithSpriteFrameName("input_bg.png");
	pwEditBg->setOpacity(0);
	loginPwEdit = CCEditBox::create(pwFieldSize, pwEditBg);
	loginPwEdit->setPosition(Vec2(
		loginPwFieldBg->getPositionX() + pwFieldSize.width * 0.5f,
		loginPwFieldBg->getPositionY()));
	loginPwEdit->setFont(FONT_NAME, 12);
	loginPwEdit->setFontColor(ccc3(255, 255, 255));
	loginPwEdit->setPlaceHolder("");
	loginPwEdit->setInputMode(kEditBoxInputModeSingleLine);
	loginPwEdit->setInputFlag(kEditBoxInputFlagPassword);
	loginPwEdit->setReturnType(kKeyboardReturnTypeDone);
	loginPwEdit->setMaxLength(64);
	loginPwEdit->setDelegate(this);
	loginLayer->addChild(loginPwEdit, 4);

	auto loginSubmitBtn = MenuItemSprite::create(
		Sprite::createWithSpriteFrameName("login_btn2.png"),
		Sprite::createWithSpriteFrameName("login_btn1.png"),
		this, menu_selector(StartMenu::onLoginLayerSubmit));
	auto closeBtn = MenuItemSprite::create(
		Sprite::createWithSpriteFrameName("no_btn1.png"),
		Sprite::createWithSpriteFrameName("no_btn2.png"),
		this, menu_selector(StartMenu::onLoginLayerClose));

	auto loginMenu = Menu::create(loginSubmitBtn, closeBtn, nullptr);
	loginMenu->setPosition(Vec2(0, 0));
	loginSubmitBtn->setPosition(Vec2(winSize.width / 2 - 40, winSize.height / 2 - 36));
	closeBtn->setPosition(Vec2(winSize.width / 2 + 40, winSize.height / 2 - 36));
	loginLayer->addChild(loginMenu, 3);

	addChild(loginLayer, 800);
	updateLoginFieldHighlight(true);
}

void StartMenu::onLoginLayerClose(Ref *sender)
{
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/cancel.wav");
	if (!loginLayer)
	{
		return;
	}

	if (loginUserEdit)
	{
		loginUserEdit->setDelegate(nullptr);
	}
	if (loginPwEdit)
	{
		loginPwEdit->setDelegate(nullptr);
	}

	loginLayer->removeAllChildren();
	loginLayer->removeFromParent();
	loginLayer = nullptr;
	loginUserEdit = nullptr;
	loginPwEdit = nullptr;
	loginUserFieldBg = nullptr;
	loginPwFieldBg = nullptr;
}

void StartMenu::onLoginLayerSubmit(Ref *sender)
{
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.wav");

	const char *userRaw = loginUserEdit ? loginUserEdit->getText() : nullptr;
	const char *pwRaw = loginPwEdit ? loginPwEdit->getText() : nullptr;
	std::string user = userRaw ? userRaw : "";
	std::string pw = pwRaw ? pwRaw : "";

	auto trimInPlace = [](std::string &s) {
		while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
		{
			s.pop_back();
		}
		while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
		{
			s.erase(s.begin());
		}
	};
	trimInPlace(user);
	trimInPlace(pw);

	if (user.empty() || pw.empty())
	{
		addChild(CCTips::create("LoginEmpty"), 5000);
		return;
	}

	std::string apiBase = UserDefault::sharedUserDefault()->getStringForKey("ns_login_api_base");
	if (apiBase.empty())
	{
		apiBase = NS_GAME_LOGIN_API_BASE_DEFAULT;
	}
	const std::string apiKey = UserDefault::sharedUserDefault()->getStringForKey("ns_login_api_key");

	const GameLoginResult res = GameLoginHttp::postLogin(apiBase, user, pw, apiKey);

	if (!res.ok)
	{
		CCLOG("Login failed: %s", res.errorMessage.c_str());
		addChild(CCTips::create("LoginFail"), 5000);
		return;
	}

	const int sqliteCoinRaw = KTools::readCoinFromSQL();
	int sqliteCoin = sqliteCoinRaw < 0 ? 0 : sqliteCoinRaw;
	if (sqliteCoin > 999999999)
		sqliteCoin = 999999999;
	const GameSyncCoinResult syncRes =
		GameLoginHttp::postSyncCoin(apiBase, user, pw, static_cast<uint32_t>(sqliteCoin), apiKey);
	std::string coinStored = res.coin;
	if (syncRes.ok)
	{
		coinStored = syncRes.coin;
	}
	else
	{
		CCLOG("Convex sync-coin (SQLite): %s", syncRes.errorMessage.c_str());
		coinStored = std::to_string(sqliteCoin);
	}

	UserDefault::sharedUserDefault()->setStringForKey("ns_account_username", user);
	UserDefault::sharedUserDefault()->setStringForKey("ns_account_game_id", res.id);
	UserDefault::sharedUserDefault()->setStringForKey("ns_account_point", res.point);
	UserDefault::sharedUserDefault()->setStringForKey("ns_account_coin", coinStored);
	UserDefault::sharedUserDefault()->setStringForKey("ns_account_group_id", res.groupId);
	UserDefault::sharedUserDefault()->flush();

	onLoginLayerClose(nullptr);
	rebuildLoginAccountMenu();
	addChild(CCTips::create("LoginSuccess"), 5000);
}

void StartMenu::editBoxEditingDidBegin(CCEditBox *editBox)
{
	if (editBox == loginUserEdit)
	{
		updateLoginFieldHighlight(true);
	}
	else if (editBox == loginPwEdit)
	{
		updateLoginFieldHighlight(false);
	}
}

void StartMenu::editBoxEditingDidEnd(CCEditBox *)
{
}

void StartMenu::editBoxTextChanged(CCEditBox *, const std::string &)
{
}

void StartMenu::editBoxReturn(CCEditBox *)
{
}

void StartMenu::updateLoginFieldHighlight(bool userActive)
{
	if (!loginUserFieldBg || !loginPwFieldBg)
	{
		return;
	}

	// Active field brighter, inactive field dimmer.
	loginUserFieldBg->setColor(userActive ? ccc3(255, 255, 255) : ccc3(180, 180, 180));
	loginPwFieldBg->setColor(userActive ? ccc3(180, 180, 180) : ccc3(255, 255, 255));
}

void StartMenu::update(float dt)
{
	if(!noticeLabel){
		return;
	}

	float currentX = noticeLabel->getPositionX();
	//float contentX = getContentSize().width;
	float lableX = noticeLabel->getContentSize().width;


	if(noticeLabel->getPositionX()>=-lableX){
		noticeLabel->setPositionX(noticeLabel->getPositionX()-0.6f);
	}	
	else{
		noticeLabel->setPositionX(190);
	}
}

void StartMenu::setNotice()
{
	if (!notice_layer)
	{
		notice_layer = Layer::create();
		Sprite *notice_bg = Sprite::createWithSpriteFrameName("notice_bg.png");
		notice_bg->setAnchorPoint(Vec2(0, 0));
		notice_bg->setPosition(Vec2(15, 228));
		notice_layer->addChild(notice_bg);

		ClippingNode *clipper = ClippingNode::create();
		Node *stencil = Sprite::createWithSpriteFrameName("notice_mask.png");
		stencil->setAnchorPoint(Vec2(0, 0));
		clipper->setStencil(stencil);

		auto strings = CCDictionary::createWithContentsOfFile("Config/strings.xml");
		auto reply = ((CCString *)strings->objectForKey("Notice"))->m_sString.c_str();

		noticeLabel = CCLabelTTF::create(reply, FONT_NAME, 12);
		noticeLabel->setAnchorPoint(Vec2(0, 0));
		clipper->addChild(noticeLabel);
		clipper->setPosition(Vec2(35, 228));

		notice_layer->addChild(clipper);

		addChild(notice_layer);
	}
}

void StartMenu::onNewsBtn(Ref *sender)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

	// JniMethodInfo minfo;

	// bool isHave = JniHelper::getStaticMethodInfo(minfo, "net/zakume/game/NarutoSenki", "getInstance", "()Lnet/zakume/game/NarutoSenki;");
	// jobject jobj;
	// if (isHave)
	// {
	// 	jobj = minfo.env->CallStaticObjectMethod(minfo.classID, minfo.methodID);

	// 	isHave = JniHelper::getMethodInfo(minfo, "net/zakume/game/NarutoSenki", "openWebview", "()V");
	// 	if (isHave)
	// 	{
	// 		minfo.env->CallVoidMethod(jobj, minfo.methodID);
	// 	}
	// }

#endif
}

void StartMenu::onHardCoreOn(Ref *sender)
{
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.wav");
	if (hardCoreLayer)
	{
		hardCoreLayer->removeAllChildren();
		hardCoreLayer->removeFromParent();
		hardCoreLayer = nullptr;
	}
}

void StartMenu::onHardCoreOff(Ref *sender)
{
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/cancel.wav");
	if (hardCoreLayer)
	{
		hardCoreLayer->removeAllChildren();
		hardCoreLayer->removeFromParent();
		hardCoreLayer = nullptr;
	}
}

void StartMenu::onHardLayerCallBack()
{
	if (UserDefault::sharedUserDefault()->getBoolForKey("isHardCore") == false)
	{
		if (!hardCoreLayer)
		{
			hardCoreLayer = Layer::create();

			Sprite *confirm_bg = Sprite::createWithSpriteFrameName("confirm_bg.png");
			confirm_bg->setPosition(Vec2(winSize.width / 2, winSize.height / 2));

			Sprite *hardcore_title = Sprite::createWithSpriteFrameName("hardcore_title.png");
			hardcore_title->setPosition(Vec2(winSize.width / 2, winSize.height / 2 + 38));

			Sprite *hardcore_text = Sprite::createWithSpriteFrameName("hardcore_text.png");
			hardcore_text->setPosition(Vec2(winSize.width / 2, winSize.height / 2 + 8));

			MenuItem *yes_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("yes_btn1.png"), Sprite::createWithSpriteFrameName("yes_btn2.png"), this, menu_selector(StartMenu::onHardCoreOn));
			MenuItem *no_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("no_btn1.png"), Sprite::createWithSpriteFrameName("no_btn2.png"), this, menu_selector(StartMenu::onHardCoreOff));

			Menu *confirm_menu = Menu::create(yes_btn, no_btn, nullptr);
			confirm_menu->alignItemsHorizontallyWithPadding(24);
			confirm_menu->setPosition(Vec2(winSize.width / 2, winSize.height / 2 - 30));

			hardCoreLayer->addChild(confirm_bg, 600);
			hardCoreLayer->addChild(confirm_menu, 650);
			hardCoreLayer->addChild(hardcore_title, 650);
			hardCoreLayer->addChild(hardcore_text, 650);
			addChild(hardCoreLayer, 700);
		}
	}
}

void StartMenu::onTrainingCallBack()
{
	// Enter game mode scene
	auto modeScene = Scene::create();
	auto gameModeLayer = GameModeLayer::create();
	modeScene->addChild(gameModeLayer);

	Director::sharedDirector()->replaceScene(TransitionFade::create(1.25f, modeScene));
}

void StartMenu::onCustomCallBack()
{
	lua_call_func(UiFlowKeys::kEnterNetworkLobby);
}

void StartMenu::onCreditsCallBack()
{
	SimpleAudioEngine::sharedEngine()->stopBackgroundMusic();
	Scene *creditsScene = Scene::create();
	CreditsLayer *creditsLayer = CreditsLayer::create();
	creditsScene->addChild(creditsLayer);
	Director::sharedDirector()->replaceScene(TransitionFade::create(1.25f, creditsScene));
}

void StartMenu::scrollMenu(int posY)
{
	if (posY > _pos02 || (isDrag && isClockwise))
	{
		for (auto menu : _menuArray)
		{
			if (menu->getPositionY() == _pos01)
			{
				auto spn = MoveTo::create(0.5, Vec2(105, _pos02));
				reorderChild(menu, 1);
				menu->_isBottom = true;
				auto fo = FadeOut::create(0.3f);
				auto seq = Spawn::createWithTwoActions(spn, fo);
				menu->runAction(seq);
			}
			else if (menu->getPositionY() == _pos03)
			{
				auto spn = Spawn::createWithTwoActions(
					MoveTo::create(0.5, Vec2(105, _pos02)),
					ScaleTo::create(0.5, 1));
				reorderChild(menu, 3);
				menu->_isTop = true;

				auto call = CallFunc::create(std::bind(&MenuButton::playSound, menu));
				auto seq = newSequence(spn, call);
				menu->runAction(seq);
			}
			else if (menu->getPositionY() == _pos02 && menu->_isBottom)
			{
				auto spn = MoveTo::create(0.5, Vec2(105, _pos03));
				reorderChild(menu, 1);
				menu->setVisible(true);
				auto fi = FadeIn::create(0.3f);
				auto seq = Spawn::createWithTwoActions(spn, fi);
				menu->_isBottom = false;
				menu->runAction(seq);
			}
			else if (menu->getPositionY() == _pos02 && !menu->_isBottom)
			{
				auto spn = Spawn::createWithTwoActions(
					MoveTo::create(0.5, Vec2(105, _pos01)),
					ScaleTo::create(0.5, 0.5));
				reorderChild(menu, 1);
				menu->_isTop = false;
				menu->runAction(spn);
			}
		}
	}
	else
	{
		for (auto menu : _menuArray)
		{
			if (menu->getPositionY() == _pos01)
			{
				auto spn = Spawn::createWithTwoActions(
					MoveTo::create(0.5, Vec2(105, _pos02)),
					ScaleTo::create(0.5, 1));
				reorderChild(menu, 3);
				menu->_isTop = true;

				auto call = CallFunc::create(std::bind(&MenuButton::playSound, menu));
				auto seq = newSequence(spn, call);
				menu->runAction(seq);
			}
			else if (menu->getPositionY() == _pos03)
			{
				auto spn = MoveTo::create(0.5, Vec2(105, _pos02));
				reorderChild(menu, 1);
				menu->_isBottom = true;
				auto fo = FadeOut::create(0.3f);
				auto seq = Spawn::createWithTwoActions(spn, fo);
				menu->runAction(seq);
			}
			else if (menu->getPositionY() == _pos02 && menu->_isBottom)
			{
				auto spn = MoveTo::create(0.5, Vec2(105, _pos01));
				reorderChild(menu, 2);
				menu->setVisible(true);
				auto fi = FadeIn::create(0.3f);
				auto seq = Spawn::createWithTwoActions(spn, fi);
				menu->_isBottom = false;
				menu->runAction(seq);
			}

			else if (menu->getPositionY() == _pos02 && !menu->_isBottom)
			{
				auto spn = Spawn::createWithTwoActions(
					MoveTo::create(0.5, Vec2(105, _pos03)),
					ScaleTo::create(0.5, 0.5));
				reorderChild(menu, 2);
				menu->_isTop = false;
				menu->runAction(spn);
			}
		}
	}

	string src;
	for (auto menu : _menuArray)
	{
		if (menu->_isTop)
		{
			switch (menu->getBtnType())
			{
			case MenuButtonType::Training:
				src = "menu02_text.png";
				break;
			case MenuButtonType::Custom:
				src = "menu01_text.png";
				break;
			case MenuButtonType::Credits:
				src = "menu04_text.png";
				break;
			case MenuButtonType::Exit:
				src = "menu03_text.png";
				break;
			default:
				break;
			}
		}
	}

	auto frame = getSpriteFrame(src);
	menuText->setDisplayFrame(frame);
}

void StartMenu::keyBackClicked()
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

	// JniMethodInfo minfo;
	// bool isHave = JniHelper::getStaticMethodInfo(minfo, "net/zakume/game/DialogUtils", "showTipDialog", "(Ljava/lang/String;Ljava/lang/String;)V");

	// if (isHave)
	// {
	// 	jstring jTitle = minfo.env->NewStringUTF("Exit Game");
	// 	jstring jMsg = minfo.env->NewStringUTF("Do you really want to exit?");
	// 	minfo.env->CallStaticVoidMethod(minfo.classID, minfo.methodID, jTitle, jMsg);
	// 	minfo.env->DeleteLocalRef(jTitle);
	// 	minfo.env->DeleteLocalRef(jMsg);
	// }
#else
	Director::sharedDirector()->end();
	exit(0);
#endif
}

void StartMenu::setCheats(int cheats)
{
	Cheats = cheats;
}

void StartMenu::onExitCallBack()
{
	keyBackClicked();
}
