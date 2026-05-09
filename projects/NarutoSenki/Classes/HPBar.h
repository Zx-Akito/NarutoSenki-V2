#pragma once
#include "CharacterBase.h"

class HPBar : public Sprite
{
public:
	bool init(const char *szImage);

	void changeBar(const char *szImage);
	void loseHP(float percent);
	/** Online-only: scale HP strip from authoritative HP (no kill/coin/death side effects). */
	void syncVisualPercent(float hpPercentRemaining);

	PROP(CharacterBase *, _delegate, Delegate);
	PROP_PTR(Sprite, hpBottom, HPBottom);
	PROP_PTR(Sprite, hpBar, HPBAR);

	static HPBar *create(const char *szImage);
};
