#pragma once
#include <string>
#include "SelectLayer.h"
#include "CreditsLayer.h"
#include "UI/GameModeLayer.h"
#include "MyUtils/KTools.h"
#include "MyUtils/CCStrokeLabel.h"
#include "MyUtils/CCScrewLayer.h"
#include "GUI/CCEditBox/CCEditBox.h"

// declare menuButton
enum class MenuButtonType
{
	Custom,
	Training,
	Exit,
	Credits,
	HardCore
};

class StartMenu;

class MenuButton : public Sprite, public CCTouchDelegate
{
public:
	bool _isTop;
	bool _isBottom;
	float prePosY;
	MenuButtonType _type;
	PROP(StartMenu *, _startMenu, Delegate);

	bool init(const char *szImage);
	CCRect getRect();
	void setBtnType(MenuButtonType type);
	MenuButtonType getBtnType();
	void playSound();

	static MenuButton *create(const char *szImage);

protected:
	void onEnter();
	void onExit();
	bool ccTouchBegan(Touch *touch, Event *event);
	void ccTouchMoved(Touch *touch, Event *event);
	void ccTouchEnded(Touch *touch, Event *event);

	inline bool containsTouchLocation(Touch *touch);
};

class StartMenu : public Layer, public cocos2d::extension::CCEditBoxDelegate
{
public:
	StartMenu();

	bool init();

	void onTrainingCallBack();
	void onCustomCallBack();
	void onHardCoreOn(Ref *sender);
	void onHardCoreOff(Ref *sender);
	void onHardLayerCallBack();

	void enterCustomMode();
	void enterTrainingMode();
	void enterSelectLayer();

	void onCreditsCallBack();
	void onExitCallBack();

	void onNewsBtn(Ref *sender);
	void onLoginBtn(Ref *sender);
	void onLoginLayerClose(Ref *sender);
	void onLoginLayerSubmit(Ref *sender);
	void updateLoginFieldHighlight(bool userActive);

	void editBoxEditingDidBegin(cocos2d::extension::CCEditBox *editBox);
	void editBoxEditingDidEnd(cocos2d::extension::CCEditBox *editBox);
	void editBoxTextChanged(cocos2d::extension::CCEditBox *editBox, const std::string &text);
	void editBoxReturn(cocos2d::extension::CCEditBox *editBox);

	void scrollMenu(int posY);
	Sprite *menuText;

	Layer *hardCoreLayer;

	bool isClockwise;
	bool isDrag;

	MenuItem *login_btn;
	Layer *loginLayer;
	cocos2d::extension::CCEditBox *loginUserEdit;
	cocos2d::extension::CCEditBox *loginPwEdit;
	Sprite *loginUserFieldBg;
	Sprite *loginPwFieldBg;
	void setNotice();

	Layer *notice_layer;
	void update(float dt);
	CCLabelTTF *noticeLabel;

	void keyBackClicked();

	void setCheats(int cheats);

	static const int _pos01 = 48;
	static const int _pos02 = 92;
	static const int _pos03 = 150;

	CREATE_FUNC(StartMenu);

private:
	void onEnter();
	void onExit();

	vector<MenuButton *> _menuArray;
};
