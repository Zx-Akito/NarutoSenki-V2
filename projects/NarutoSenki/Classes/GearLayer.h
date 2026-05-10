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
	void ccTouchCancelled(Touch *touch, Event *event);

	inline bool containsTouchLocation(Touch *touch);

private:
	// Tracks finger position to differentiate a tap from a scroll gesture
	// so that swipes on a gear button still scroll the list instead of
	// being trapped as a click.
	Vec2 _touchStartPos;
	float _lastTouchY = 0.0f;
	bool _isScrolling = false;
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

	// Apply a vertical scroll delta to the gear list and the side scroll bar.
	// Exposed so child gear buttons can forward scroll gestures here when the
	// user drags on top of a button instead of an empty area.
	void scrollByDelta(float distanceY);
	// Snap scroll position back into valid bounds after the finger is lifted.
	void clampAfterRelease();

	CREATE_FUNC(ScrewLayer);

protected:
	bool ccTouchBegan(Touch *touch, Event *event);
	void ccTouchMoved(Touch *touch, Event *event);
	void ccTouchEnded(Touch *touch, Event *event);

	void onEnter();
	void onExit();
};