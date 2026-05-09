#pragma once
#include "Defines.h"

class GameLayer;
class ScrewLayer;

enum class GearButtonType : uint8_t
{
	Buy,
	Sell
};

class GearLayer : public Layer
{
public:
	GearLayer();
	~GearLayer();

	bool init(RenderTexture *snapshoot, bool overlayLiveBackdrop = false);

	Layer *gears_layer = nullptr;
	Layer *currentGear_layer = nullptr;
	CCLabelBMFont *coinLabel = nullptr;

	Sprite *gearDetail = nullptr;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX) || (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	Sprite *gearBigIcon;
#endif
	Sprite *gears_bg = nullptr;

	ScrewLayer *_screwLayer = nullptr;
	GearType currentGear = GearType::None;
	void updatePlayerGear();
	void updateGearList();
	void confirmPurchase();

	static GearLayer *create(RenderTexture *snapshoot, bool overlayLiveBackdrop = false);

private:
	void onResume(Ref *sender);
	void onGearBuy(Ref *sender);
};

class GearButton : public Sprite, public CCTouchDelegate
{
public:
	bool init(const char *szImage);

	bool _isBuyed;
	GearType _gearType;
	GearButtonType _btnType;
	Sprite *soIcon;
	PROP(GearLayer *, _delegate, Delegate);

	CCRect getRect();
	void setBtnType(GearType type, GearButtonType btnType, bool isBuyed);
	GearType getBtnType();
	void playSound();
	void click();

	static GearButton *create(const char *szImage);

protected:
	void onEnter();
	void onExit();
	bool ccTouchBegan(Touch *touch, Event *event);
	void ccTouchMoved(Touch *touch, Event *event);
	void ccTouchEnded(Touch *touch, Event *event);

	inline bool containsTouchLocation(Touch *touch);
};

class ScrewLayer : public Layer
{
public:
	bool init();

	float prePosY;
	int totalRow;
	int gearNum;
	Sprite *screwBar;
	PROP_Vector(vector<GearButton *>, _gearBtnArray, GearBtnArray);
	PROP(GearLayer *, _delegate, Delegate);

	CREATE_FUNC(ScrewLayer);

protected:
	bool ccTouchBegan(Touch *touch, Event *event);
	void ccTouchMoved(Touch *touch, Event *event);
	void ccTouchEnded(Touch *touch, Event *event);

	void onEnter();
	void onExit();
};