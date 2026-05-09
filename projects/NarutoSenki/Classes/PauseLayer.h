#pragma once
#include "Defines.h"

class PauseLayer : public Layer
{
public:
	/** snapshot = frozen frame behind menu; omit when overlayLiveBackdrop (e.g. online match keeps battle animating beneath). */
	bool init(RenderTexture *snapshoot, bool overlayLiveBackdrop = false);

	Menu *pauseMenu = nullptr;
	Menu *soundMenu = nullptr;
	Menu *preMenu = nullptr;
	Layer *exitLayer = nullptr;
	MenuItem *bgm_btn = nullptr;
	MenuItem *voice_btn = nullptr;
	MenuItem *pre_btn = nullptr;

	static PauseLayer *create(RenderTexture *snapshoot, bool overlayLiveBackdrop = false);

private:
	void onResume(Ref *sender);
	void onBackToMenu(Ref *sender);
	void onLeft(Ref *sender);
	void onCancel(Ref *sender);
	void onBGM(Ref *sender);
	void onPreload(Ref *sender);
	void onVoice(Ref *sender);
};
