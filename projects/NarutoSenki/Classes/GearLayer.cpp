#include "GearLayer.h"
#include "GameLayer.h"
#include "HudLayer.h"
#include "Core/Hero.hpp"

/*----------------------
init GearButton ;
----------------------*/

bool GearButton::init(const char *szImage)
{
	RETURN_FALSE_IF(!Sprite::init());

	if (!is_same(szImage, ""))
		initWithSpriteFrameName(szImage);

	setAnchorPoint(Vec2(0, 0));

	return true;
}

void GearButton::onEnter()
{
	Sprite::onEnter();
	Director::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -1, true);
}

void GearButton::onExit()
{
	Sprite::onExit();
	Director::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
}

CCRect GearButton::getRect()
{
	CCSize size = getContentSize();
	return CCRect(0, 0, size.width, size.height);
}

bool GearButton::containsTouchLocation(Touch *touch)
{
	return getRect().containsPoint(convertTouchToNodeSpace(touch));
}

void GearButton::setBtnType(GearType type, GearButtonType btnType, bool isBuyed)
{
	_gearType = type;
	_btnType = btnType;

	if (_btnType == GearButtonType::Buy)
	{
		Sprite *gearIcon = Sprite::createWithSpriteFrameName(format("gear_{:02d}.png", (int)_gearType).c_str());
		gearIcon->setPosition(Vec2(20, 30));
		addChild(gearIcon);
	}
	else
	{
		Sprite *gearIcon = Sprite::createWithSpriteFrameName(format("gear_{:02d}.png", (int)_gearType).c_str());
		gearIcon->setScale(0.75f);
		addChild(gearIcon);
	}

	if (isBuyed)
	{
		_isBuyed = true;
		soIcon = Sprite::createWithSpriteFrameName("gear_so.png");
		soIcon->setPosition(Vec2(getContentSize().width / 2, getContentSize().height / 2));
		addChild(soIcon);
	}
}

GearType GearButton::getBtnType()
{
	return _gearType;
}

void GearButton::click()
{
	if (_delegate->currentGear != _gearType && UserDefault::sharedUserDefault()->getBoolForKey("isVoice"))
	{
		SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/select.ogg");
	}

	if (!_isBuyed)
	{
		_delegate->currentGear = _gearType;
	}

	auto frame = getSpriteFrame("gearDetail_{:02d}.png", (int)_gearType);
	_delegate->gearDetail->setDisplayFrame(frame);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX) || (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	auto icon = getSpriteFrame("gear_{:02d}.png", (int)_gearType);
	_delegate->gearBigIcon->setDisplayFrame(icon);
#endif
}

bool GearButton::ccTouchBegan(Touch *touch, Event *event)
{
	// touch area
	if (!containsTouchLocation(touch) || soIcon)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void GearButton::ccTouchMoved(Touch *touch, Event *event)
{
	// touch area
}

void GearButton::ccTouchEnded(Touch *touch, Event *event)
{
	click();
}

void GearButton::playSound()
{
}

GearButton *GearButton::create(const char *szImage)
{
	GearButton *mb = new GearButton();
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

bool ScrewLayer::init()
{
	RETURN_FALSE_IF(!Layer::init());

	return true;
}

void ScrewLayer::onEnter()
{
	Layer::onEnter();
	Director::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, 0, true);
}

void ScrewLayer::onExit()
{
	Layer::onExit();
	Director::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
}

bool ScrewLayer::ccTouchBegan(Touch *touch, Event *event)
{
	prePosY = 0;
	return true;
}

void ScrewLayer::ccTouchMoved(Touch *touch, Event *event)
{
	// touch area
	Vec2 curPoint = touch->getLocation();
	if (prePosY == 0)
	{
		prePosY = curPoint.y;
	}
	else
	{
		float distanceY = curPoint.y - prePosY;
		if (getPositionY() < totalRow * 74 || distanceY < 0)
		{
			setPositionY(getPositionY() + distanceY);
		}

		if ((screwBar->getPositionY() > 90 || distanceY < 0) && screwBar->getPositionY() <= 122)
		{
			screwBar->setPositionY(screwBar->getPositionY() - distanceY);
		}

		if (screwBar->getPositionY() > 122)
		{
			screwBar->setPositionY(122);
		}

		if (screwBar->getPositionY() < 90)
		{
			screwBar->setPositionY(90);
		}

		prePosY = curPoint.y;
	}
};

void ScrewLayer::ccTouchEnded(Touch *touch, Event *event)
{
	prePosY = 0;
	// CCLOG("%f",getPositionY());
	// CCLOG("Height:%f",getContentSize().height);

	if (getPositionY() > totalRow * 74 - 74)
	{
		setPositionY(totalRow * 65);
	}

	if (getPositionY() < 76)
	{
		setPositionY(76);
	}

	if (screwBar->getPositionY() > 122)
	{
		screwBar->setPositionY(122);
	}

	if (screwBar->getPositionY() < 90)
	{
		screwBar->setPositionY(90);
	}
}

GearLayer::GearLayer()
{
}

GearLayer::~GearLayer()
{
	getGameLayer()->_isGear = false;
}

bool GearLayer::init(RenderTexture *snapshoot, bool overlayLiveBackdrop)
{
	RETURN_FALSE_IF(!Layer::init());

	if (!overlayLiveBackdrop)
	{
		SimpleAudioEngine::sharedEngine()->stopAllEffects();
	}

	if (!overlayLiveBackdrop)
	{
		if (!snapshoot || !snapshoot->getSprite())
			return false;
		Texture2D *bgTexture = snapshoot->getSprite()->getTexture();
		Sprite *bg = Sprite::createWithTexture(bgTexture);
		bg->setAnchorPoint(Vec2(0, 0));
		bg->setFlipY(true);
		addChild(bg, 0);
	}

	Layer *blend = LayerColor::create(ccc4(0, 0, 0, 150), winSize.width, winSize.height);
	addChild(blend, 1);

	Layer *gears_layer = Layer::create();

	gears_bg = Sprite::createWithSpriteFrameName("gears_bg.png");
	gears_bg->setPosition(Vec2(winSize.width / 2, winSize.height / 2 - 12));
	gears_layer->addChild(gears_bg, 1);

	coinLabel = CCLabelBMFont::create("0", Fonts::Arial);
	coinLabel->setAnchorPoint(Vec2(0, 0));
	coinLabel->setPosition(Vec2(gears_bg->getPositionX() + 2, 58));
	gears_layer->addChild(coinLabel, 2);

	addChild(gears_layer, 10);

	ClippingNode *clipper = ClippingNode::create();
	Node *stencil = Sprite::createWithSpriteFrameName("screwMask.png");
	stencil->setAnchorPoint(Vec2(0, 0));
	clipper->setStencil(stencil);

	_screwLayer = ScrewLayer::create();
	_screwLayer->setAnchorPoint(Vec2(0, 0));
	_screwLayer->setPositionY(76);
	_screwLayer->gearNum = 9;

	_screwLayer->screwBar = Sprite::createWithSpriteFrameName("screwBar.png");
	_screwLayer->screwBar->setAnchorPoint(Vec2(0.5f, 0));
	_screwLayer->screwBar->setPosition(Vec2(gears_bg->getPositionX() + 25, 126));
	addChild(_screwLayer->screwBar, 600);

	gearDetail = Sprite::createWithSpriteFrameName("gearDetail_00.png");
	gearDetail->setAnchorPoint(Vec2(0.5f, 1));
	gearDetail->setPosition(Vec2(gears_bg->getPositionX() + gears_bg->getContentSize().width / 2 - 54, 210));
	addChild(gearDetail, 600);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX) || (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	gearBigIcon = Sprite::createWithSpriteFrameName("gear_00.png");
	gearBigIcon->setAnchorPoint(Vec2(0.5f, 0));
	gearBigIcon->setPosition(Vec2(gears_bg->getPositionX() + gears_bg->getContentSize().width / 2 - 54, 90));
	addChild(gearBigIcon, 600);
#endif

	MenuItem *buy_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("gearBuy_btn.png"),
											   Sprite::createWithSpriteFrameName("gearBuy_btn2.png"), this, menu_selector(GearLayer::onGearBuy));
	Menu *gearMenu = Menu::create(buy_btn, nullptr);
	gearMenu->setPosition(Vec2(gears_bg->getPositionX() + 78, 65));
	addChild(gearMenu, 600);
	clipper->setPosition(Vec2(gears_bg->getPositionX() - gears_bg->getContentSize().width / 2 + 4, 85));
	clipper->addChild(_screwLayer);
	addChild(clipper, 600);

	MenuItem *btm_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("close_btn1.png"),
											   Sprite::createWithSpriteFrameName("close_btn2.png"), this, menu_selector(GearLayer::onResume));
	Menu *overMenu = Menu::create(btm_btn, nullptr);
	overMenu->setPosition(Vec2(winSize.width / 2 + gears_bg->getContentSize().width / 2 - 12, winSize.height / 2 + gears_bg->getContentSize().height / 2 - 20));
	addChild(overMenu, 600);

	return true;
}

void GearLayer::confirmPurchase()
{
	// this function for keyboard buy event
	onGearBuy(nullptr);
	// refresh HUB
	getGameLayer()->getHudLayer()->updateGears();
}

void GearLayer::onResume(Ref *sender)
{
	getGameLayer()->dismissGearOverlay();
}

void GearLayer::onGearBuy(Ref *sender)
{
	if (UserDefault::sharedUserDefault()->getBoolForKey("isVoice"))
	{
		SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.ogg");
	}

	if (getGameLayer()->currentPlayer->setGear(currentGear))
	{
		updatePlayerGear();
	}
}

void GearLayer::updatePlayerGear()
{
	if (getGameLayer()->currentPlayer->getGearArray().size() > 0)
	{
		if (currentGear_layer != nullptr)
			currentGear_layer->removeFromParent();
		currentGear_layer = Layer::create();
		currentGear_layer->setAnchorPoint(Vec2(0, 0));
		int i = 0;
		for (auto gear : getGameLayer()->currentPlayer->getGearArray())
		{
			GearButton *btn = GearButton::create("");
			btn->setBtnType(gear, GearButtonType::Sell, false);
			btn->setPositionX(13 + i * 34);
			btn->setDelegate(this);
			currentGear_layer->addChild(btn);
			i++;
		}
		currentGear_layer->setPosition(Vec2(gears_bg->getPositionX() - gears_bg->getContentSize().width / 2 + 8, 64));
		addChild(currentGear_layer, 800);
	}

	coinLabel->setString(to_cstr(getGameLayer()->currentPlayer->getCoin()));
	updateGearList();
}

void GearLayer::updateGearList()
{
	auto &gearBtns = _screwLayer->getGearBtnArray();
	if (!gearBtns.empty())
	{
		for (auto btn : gearBtns)
			btn->removeFromParent();
		gearBtns.clear();
	}

	currentGear = GearType::None;
	_screwLayer->totalRow = 3;
	for (uint8_t i = 0; i < _screwLayer->gearNum; i++)
	{
		int row = floor(i / 3.0f);
		int column = abs(3 * row - i);

		GearButton *btn;
		if (column >= 1)
		{
			btn = GearButton::create("value_1000.png");
		}
		else
		{
			btn = GearButton::create("value_500.png");
		}

		bool isBuyed = false;
		for (auto gear : getGameLayer()->currentPlayer->getGearArray())
		{
			if (static_cast<uint32_t>(gear) == i)
				isBuyed = true;
		}

		if (currentGear == GearType::None && !isBuyed)
		{
			currentGear = GearType(i);
			auto frame = getSpriteFrame("gearDetail_{:02d}.png", i);
			gearDetail->setDisplayFrame(frame);
		}

		btn->setPosition(Vec2(6 + column * 46, -row * 60));
		btn->setAnchorPoint(Vec2(0, 0));
		btn->setBtnType(GearType(i), GearButtonType::Buy, isBuyed);
		btn->setDelegate(this);

		gearBtns.push_back(btn);
		_screwLayer->addChild(btn);
	}
}

GearLayer *GearLayer::create(RenderTexture *snapshoot, bool overlayLiveBackdrop)
{
	GearLayer *grl = new GearLayer();
	if (grl && grl->init(snapshoot, overlayLiveBackdrop))
	{
		grl->autorelease();
		return grl;
	}
	else
	{
		delete grl;
		return nullptr;
	}
}
