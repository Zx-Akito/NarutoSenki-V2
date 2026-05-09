#include "GameOver.h"
#include "GameLayer.h"
#include "Core/Hero.hpp"
#include "GameMode/GameModeImpl.h"

GameOver::GameOver()
{
	_isWin = false;
	finnalScore = 0;
}

GameOver::~GameOver()
{
	AnimationCache::purgeSharedAnimationCache();
	SpriteFrameCache::sharedSpriteFrameCache()->removeUnusedSpriteFrames();
	TextureCache::sharedTextureCache()->removeUnusedTextures();
}

bool GameOver::init(RenderTexture *snapshoot)
{
	RETURN_FALSE_IF(!Layer::init());

	SimpleAudioEngine::sharedEngine()->stopAllEffects();
	SimpleAudioEngine::sharedEngine()->stopBackgroundMusic(true);

	auto bgTexture = snapshoot->getSprite()->getTexture();
	auto bg = Sprite::createWithTexture(bgTexture);
	bg->setAnchorPoint(Vec2(0, 0));
	bg->setFlipY(true);
	addChild(bg, 0);

	auto blend = LayerColor::create(ccc4(0, 0, 0, 150), winSize.width, winSize.height);
	addChild(blend, 1);

	// produce the menu_bar
	auto menu_bar_b = Sprite::create("menu_bar2.png");
	menu_bar_b->setAnchorPoint(Vec2(0, 0));
	FULL_SCREEN_SPRITE(menu_bar_b);
	addChild(menu_bar_b, 2);

	auto menu_bar_t = Sprite::create("menu_bar3.png");
	menu_bar_t->setAnchorPoint(Vec2(0, 0));
	menu_bar_t->setPosition(Vec2(0, winSize.height - menu_bar_t->getContentSize().height));
	FULL_SCREEN_SPRITE(menu_bar_t);
	addChild(menu_bar_t, 2);

	auto result_title = Sprite::createWithSpriteFrameName("result_title.png");
	result_title->setAnchorPoint(Vec2(0, 0));
	result_title->setPosition(Vec2(2, winSize.height - result_title->getContentSize().height - 2));
	addChild(result_title, 3);

	result_bg = Sprite::createWithSpriteFrameName("gameover_bg.png");
	result_bg->setScale(0.5f);
	result_bg->setPosition(Vec2(winSize.width / 2, winSize.height / 2 - 6));
	addChild(result_bg, 4);

	auto su = ScaleTo::create(0.2f, 1.0);
	auto call = CallFunc::create(std::bind(&GameOver::listResult, this));
	auto seq = newSequence(su, call);
	result_bg->runAction(seq);

	return true;
}

void GameOver::listResult()
{
	if (getGameLayer()->_isHardCoreGame)
		SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/battle_over1.ogg");

	else
		SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/battle_over.ogg");

	auto currPlayer = getGameLayer()->currentPlayer;
	auto half = Sprite::createWithSpriteFrameName(format("{}_half.png", currPlayer->getName()).c_str());

	if (currPlayer->getName() == HeroEnum::Konan ||
		currPlayer->getName() == HeroEnum::Karin ||
		currPlayer->getName() == HeroEnum::Suigetsu ||
		currPlayer->getName() == HeroEnum::Hidan ||
		currPlayer->getName() == HeroEnum::Tobirama ||
		currPlayer->getName() == HeroEnum::Tsunade ||
		currPlayer->getName() == HeroEnum::Kankuro ||
		currPlayer->getName() == HeroEnum::SageJiraiya ||
		currPlayer->getName() == HeroEnum::Minato ||
		currPlayer->getName() == HeroEnum::Tobi ||
		currPlayer->getName() == HeroEnum::Lee ||
		currPlayer->getName() == HeroEnum::RockLee ||
		currPlayer->getName() == HeroEnum::Hinata ||
		currPlayer->getName() == HeroEnum::Asuma ||
		currPlayer->getName() == HeroEnum::Chiyo ||
		currPlayer->getName() == HeroEnum::Kisame)
	{
		half->setFlipX(true);
	}

	half->setAnchorPoint(Vec2(0, 0));
	half->setPosition(Vec2(winSize.width / 2 + result_bg->getContentSize().width / 2 - half->getContentSize().width, result_bg->getPositionY() - result_bg->getContentSize().height / 2 + 9));
	addChild(half, 5);

	auto list_bg = Sprite::createWithSpriteFrameName("list_bg.png");
	list_bg->setAnchorPoint(Vec2(0, 0));
	list_bg->setPosition(Vec2(winSize.width / 2 - result_bg->getContentSize().width / 2 + 2, result_bg->getPositionY() - result_bg->getContentSize().height / 2 + 26));
	addChild(list_bg, 5);

	int _hour = getGameLayer()->_minute / 60;
	int _minute = getGameLayer()->_minute % 60;

	auto timeBG = Sprite::createWithSpriteFrameName("time_bg.png");
	timeBG->setAnchorPoint(Vec2(0, 0));
	timeBG->setPosition(Vec2(winSize.width / 2 + result_bg->getContentSize().width / 2 - 11 - timeBG->getContentSize().width, result_bg->getPositionY() - result_bg->getContentSize().height / 2 + 46));
	addChild(timeBG, 6);

	auto tempTime = format("{:02d}:{:02d}:{:02d}", _hour, _minute, getGameLayer()->_second);
	auto gameClock = CCLabelBMFont::create(tempTime.c_str(), Fonts::Default);
	gameClock->setAnchorPoint(Vec2(0.5f, 0));
	gameClock->setPosition(Vec2(timeBG->getPositionX() + timeBG->getContentSize().width / 2, timeBG->getPositionY() + 3));
	gameClock->setScale(0.48f);
	addChild(gameClock, 7);

	uint32_t _totalSecond = getGameLayer()->_minute * 60 + getGameLayer()->_second;
	float resultScore = 0;
	uint32_t killDead = currPlayer->getKillNum() - currPlayer->_deadNum;

	if (getGameLayer()->_isHardCoreGame)
	{
		if (_totalSecond > 900)
			resultScore = ((killDead / (_totalSecond / 60.0f)) / 3) * 100;
		else
			resultScore = ((killDead - ((_totalSecond / 60.0f - 15) * 3)) / 45) * 100;
	}
	else
	{
		if (_totalSecond > 600)
			resultScore = ((killDead / (_totalSecond / 60.0f)) / 4) * 100;
		else
			resultScore = ((killDead - ((_totalSecond / 60.0f - 10) * 4)) / 40) * 100;
	}

	int i = 0;
	uint32_t konohaKill = 0;
	uint32_t akatsukiKill = 0;

	for (auto hero : getGameLayer()->_CharacterArray)
	{
		if (hero->isClone() ||
			hero->isSummon() ||
			hero->isKugutsu() ||
			hero->isGuardian())
		{
			continue;
		}

		// 4v4
		if (Cheats >= kMaxCheats)
		{
			if (hero->isNotPlayer())
			{
				continue;
			}
		}

		if (hero->_isControlled)
		{
			hero->_isControlled = false;
			hero->changeGroup();
		}

		auto avator_small = Sprite::createWithSpriteFrameName(format("{}_small.png", hero->getName()).c_str());
		avator_small->setAnchorPoint(Vec2(0, 0));

		uint32_t realKillNum = hero->getKillNum();

		if (hero->isKonohaGroup())
		{
			konohaKill += realKillNum;
		}
		else
		{
			akatsukiKill += realKillNum;
		}

		auto killNum = CCLabelBMFont::create(to_cstr(realKillNum), Fonts::Default);
		killNum->setScale(0.3f);

		auto deadNum = CCLabelBMFont::create(to_cstr(hero->_deadNum), Fonts::Default);
		deadNum->setScale(0.3f);

		auto flogNum = CCLabelBMFont::create(to_cstr(hero->_flogNum), Fonts::Default);
		flogNum->setScale(0.3f);

		float posX = winSize.width / 2 - result_bg->getContentSize().width / 2 + 2;

		if (hero->isKonohaGroup())
		{
			avator_small->setPosition(Vec2(posX, result_bg->getPositionY() - result_bg->getContentSize().height / 2 + 152 - (i * 25)));
			killNum->setPosition(Vec2(posX + 44, avator_small->getPositionY() + avator_small->getContentSize().height / 2));
			deadNum->setPosition(Vec2(posX + 68, avator_small->getPositionY() + avator_small->getContentSize().height / 2));
			flogNum->setPosition(Vec2(posX + 94, avator_small->getPositionY() + avator_small->getContentSize().height / 2));
		}
		else
		{
			avator_small->setPosition(Vec2(posX, result_bg->getPositionY() - result_bg->getContentSize().height / 2 + 76 - ((i - 3) * 25)));
			killNum->setPosition(Vec2(posX + 44, avator_small->getPositionY() + avator_small->getContentSize().height / 2));
			deadNum->setPosition(Vec2(posX + 68, avator_small->getPositionY() + avator_small->getContentSize().height / 2));
			flogNum->setPosition(Vec2(posX + 94, avator_small->getPositionY() + avator_small->getContentSize().height / 2));
		}

		if (hero->getGearArray().size() > 0)
		{
			int j = 0;
			for (auto gear : hero->getGearArray())
			{
				auto gearIcon = Sprite::createWithSpriteFrameName(format("gear_{:02d}.png", (int)gear).c_str());
				gearIcon->setPosition(Vec2(flogNum->getPositionX() + 22 + j * 19, flogNum->getPositionY() - 1));
				gearIcon->setScale(0.5f);
				addChild(gearIcon, 7);
				j++;
			}
		}

		addChild(avator_small, 7);
		addChild(killNum, 7);
		addChild(deadNum, 7);
		addChild(flogNum, 7);

		i++;
	}

	if (_totalSecond > 900 && getGameLayer()->_isSurrender)
	{
		if (currPlayer->getGroup() == Group::Konoha)
		{
			if (konohaKill > akatsukiKill)
				_isWin = true;
		}
		else
		{
			if (akatsukiKill > konohaKill)
				_isWin = true;
		}
	}

	if (Cheats < kMaxCheats)
	{
		uint32_t realKillNum = currPlayer->getKillNum();

		string tempReward = getGameLayer()->_isHardCoreGame ? "FDDD\24" : "ONNN\24";
		KTools::decode(tempReward);

		uint32_t coins = KTools::readCoinFromSQL();
		uint32_t rewardNum;
		if (_isWin)
			rewardNum = realKillNum * 75 + to_int(tempReward.c_str());
		else
			rewardNum = realKillNum * 50;

		auto coinBG = Sprite::createWithSpriteFrameName("coin_bg.png");
		coinBG->setAnchorPoint(Vec2(0, 0));
		coinBG->setPosition(Vec2(winSize.width / 2 + result_bg->getContentSize().width / 2 - coinBG->getContentSize().width - 11, result_bg->getPositionY() - result_bg->getContentSize().height / 2 + 12));
		addChild(coinBG, 6);

		auto adExtra = Sprite::createWithSpriteFrameName("adExtra.png");
		adExtra->setAnchorPoint(Vec2(0.5f, 0));
		adExtra->setPosition(Vec2(coinBG->getPositionX() + 70, coinBG->getPositionY() + 22));
		addChild(adExtra, 7);

		const char *extraCoin;
		uint32_t tempCoin;
		if (_isWin)
		{
			string tempEtra = "idd4";
			KTools::decode(tempEtra);
			tempCoin = rewardNum + coins + to_int(tempEtra.c_str());
			extraCoin = "+500";
		}
		else
		{
			tempCoin = rewardNum + coins;
			extraCoin = "+0";
		}

		KTools::saveToSQLite("GameRecord", "coin", std::to_string(tempCoin).c_str(), false);

		auto extraLabel = CCLabelBMFont::create(extraCoin, Fonts::Yellow);
		extraLabel->setScale(0.5f);
		extraLabel->setAnchorPoint(Vec2(0.5f, 0));
		extraLabel->setPosition(Vec2(coinBG->getPositionX() + 68, coinBG->getPositionY() + 3));
		addChild(extraLabel, 7);

		auto rewardCoin = to_cstr(rewardNum);
		auto rewardLabel = CCLabelBMFont::create(rewardCoin, Fonts::Yellow);
		rewardLabel->setAnchorPoint(Vec2(0.5f, 0));
		rewardLabel->setPosition(Vec2(coinBG->getPositionX() + 28, coinBG->getPositionY() + 3));
		rewardLabel->setScale(0.55f);
		addChild(rewardLabel, 7);
	}

	const char *imgSrc = nullptr;
	bool isEnableSROrBetter = getGameLayer()->_isHardCoreGame && getGameLayer()->_isRandomChar && !getGameLayer()->_enableGear;

	if (_isWin)
	{
		if (resultScore >= 140 && isEnableSROrBetter)
			imgSrc = "result_SSSR.png";
		else if (resultScore >= 120 && isEnableSROrBetter)
			imgSrc = "result_SSR.png";
		else if (resultScore >= 100 && isEnableSROrBetter)
			imgSrc = "result_SR.png";
		else if (resultScore >= 140)
			imgSrc = "result_SSS.png";
		else if (resultScore >= 120)
			imgSrc = "result_SS.png";
		else if (resultScore >= 100)
			imgSrc = "result_S.png";
		else if (resultScore >= 80)
			imgSrc = "result_A.png";
		else if (resultScore >= 60)
			imgSrc = "result_B.png";
		else
			imgSrc = "result_C.png";
	}
	else if (!_isWin)
	{
		imgSrc = "result_Defeat.png";
	}

	if (imgSrc)
	{
		auto recordSprite = Sprite::createWithSpriteFrameName(imgSrc);
		recordSprite->setAnchorPoint(Vec2(0, 0));
		recordSprite->setPosition(Vec2(winSize.width / 2 + result_bg->getContentSize().width / 2 - recordSprite->getContentSize().width - 12, result_bg->getPositionY() - result_bg->getContentSize().height / 2 + 88));
		addChild(recordSprite, 7);

		if (_isWin && getGameLayer()->_isHardCoreGame)
		{
			// finnalScore = resultScore + float(currPlayer->_flogNum) / 100;
			// auto recordString = format("{0.2f}Pts", finnalScore);
			// auto recordScore = CCLabelBMFont::create(recordString.c_str(), Fonts::Default);
			// recordScore->setAnchorPoint(Vec2(1, 0.5f));
			// recordScore->setPosition(Vec2(recordSprite->getPositionX() + recordSprite->getContentSize().width, recordSprite->getPositionY() - 7));
			// recordScore->setScale(0.35f);
			// addChild(recordScore, 10);

			// upload_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("upload_btn.png"), Sprite::createWithSpriteFrameName("upload_btn.png"), nullptr, this, menu_selector(GameOver::onUPloadBtn));
			// Menu *upMenu = Menu::create(upload_btn, nullptr);
			// upload_btn->setAnchorPoint(Vec2(1.0, 0));
			// upMenu->setPosition(Vec2(winSize.width / 2 + result_bg->getContentSize().width / 2 - 14, winSize.height / 2 + result_bg->getContentSize().height / 2 - 62));
			// addChild(upMenu, 7);

			// detailRecord = format("{:02d}:{:02d},{},{},{}", _minute, getGameLayer()->_second, currPlayer->getKillNum(), currPlayer->_deadNum, currPlayer->_flogNum);
		}
		if (Cheats < kMaxCheats)
		{
			resultChar = currPlayer->getName().c_str();
			if (currPlayer->getName() == HeroEnum::SageNaruto)
				resultChar = HeroEnum::Naruto;
			else if (currPlayer->getName() == HeroEnum::RikudoNaruto)
				resultChar = HeroEnum::Naruto;
			else if (currPlayer->getName() == HeroEnum::SageJiraiya)
				resultChar = HeroEnum::Jiraiya;
			else if (currPlayer->getName() == HeroEnum::ImmortalSasuke)
				resultChar = HeroEnum::Sasuke;
			else if (currPlayer->getName() == HeroEnum::RockLee)
				resultChar = HeroEnum::Lee;
			else if (currPlayer->getName() == HeroEnum::Nagato)
				resultChar = HeroEnum::Pain;

			if (_isWin)
			{
				int winNum = KTools::readWinNumFromSQL(resultChar);
				if (resultScore >= 140)
					winNum += 3;
				else if (resultScore >= 120)
					winNum += 2;
				else
					winNum += 1;

				auto realWin = std::to_string(winNum);
				KTools::saveSQLite("CharRecord", "name", resultChar, "column1", realWin, false);

				if (getGameLayer()->_isRandomChar && resultScore >= 120)
				{
					if (currPlayer->_isControlled)
					{
						currPlayer->_isControlled = false;
						currPlayer->changeGroup();
					}

					for (auto hero : getGameLayer()->_CharacterArray)
					{
						if (hero->isClone() ||
							hero->isPlayer() ||
							hero->isSummon() ||
							hero->isKugutsu() ||
							hero->isGuardian())
						{
							continue;
						}

						if (hero->_isControlled)
						{
							hero->_isControlled = false;
							hero->changeGroup();
						}

						if (hero->getGroup() == currPlayer->getGroup())
						{
							int winNum2 = KTools::readWinNumFromSQL(hero->getName().c_str());
							if (resultScore >= 140)
								winNum2 += 2;
							else
								winNum2 += 1;

							auto realWin2 = std::to_string(winNum2);
							KTools::saveSQLite("CharRecord", "name", hero->getName().c_str(), "column1", realWin2, false);
						}
					}
				}

				auto bestTime = KTools::readSQLite("CharRecord", "name", resultChar, "column3");
				if (bestTime.empty())
				{
					KTools::saveSQLite("CharRecord", "name", resultChar, "column3", tempTime, false);
				}
				else
				{
					int recordHour = to_int(bestTime.substr(0, 2).c_str());
					int recordMinute = to_int(bestTime.substr(3, 2).c_str());
					int recordSecond = to_int(bestTime.substr(6, 2).c_str());

					int recordTime = recordHour * 60 * 60 + recordMinute * 60 + recordSecond;
					int currentTime = getGameLayer()->_minute * 60 + getGameLayer()->_second;
					bool isNewRecord = currentTime < recordTime;

					if (isNewRecord)
					{
						KTools::saveSQLite("CharRecord", "name", resultChar, "column3", tempTime, false);
					}
				}
			}
		}
	}

	// auto version = CCLabelBMFont::create(Cheats < kMaxCheats ? GAMEOVER_VER : GAMEOVER_VER, Fonts::Default);
	auto version = CCLabelBMFont::create(GAMEOVER_VER, Fonts::Default);
	version->setPosition(Vec2(winSize.width / 2 + 94, result_bg->getPositionY() - result_bg->getContentSize().height / 2 + 6));
	version->setScale(0.3f);
	addChild(version, 5);

	MenuItem *btm_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("close_btn1.png"), Sprite::createWithSpriteFrameName("close_btn2.png"), nullptr, this, menu_selector(GameOver::onBackToMenu));
	Menu *overMenu = Menu::create(btm_btn, nullptr);
	overMenu->setPosition(Vec2(winSize.width / 2 + result_bg->getContentSize().width / 2 - 12, winSize.height / 2 + result_bg->getContentSize().height / 2 - 18));
	addChild(overMenu, 7);

	getGameLayer()->_isSurrender = false;

	getGameModeHandler()->onGameOver();
}

void GameOver::onUPloadBtn(Ref *sender)
{
	auto tip = CCTips::create("ServerMainten");
	addChild(tip, 5000);
}

void GameOver::onBackToMenu(Ref *sender)
{
	if (!exitLayer)
	{
		SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/select.ogg");
		exitLayer = Layer::create();

		auto exit_bg = Sprite::createWithSpriteFrameName("confirm_bg.png");
		exit_bg->setPosition(Vec2(winSize.width / 2, winSize.height / 2));

		auto comfirm_title = Sprite::createWithSpriteFrameName("confirm_title.png");
		comfirm_title->setPosition(Vec2(winSize.width / 2, winSize.height / 2 + 38));

		auto btm_text = Sprite::createWithSpriteFrameName("btm_text.png");
		btm_text->setPosition(Vec2(winSize.width / 2, winSize.height / 2 + 8));

		MenuItem *yes_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("yes_btn1.png"), Sprite::createWithSpriteFrameName("yes_btn2.png"), this, menu_selector(GameOver::onLeft));
		MenuItem *no_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("no_btn1.png"), Sprite::createWithSpriteFrameName("no_btn2.png"), this, menu_selector(GameOver::onCancel));

		Menu *confirm_menu = Menu::create(yes_btn, no_btn, nullptr);
		confirm_menu->alignItemsHorizontallyWithPadding(24);
		confirm_menu->setPosition(Vec2(winSize.width / 2, winSize.height / 2 - 30));

		exitLayer->addChild(exit_bg, 1);
		exitLayer->addChild(confirm_menu, 2);
		exitLayer->addChild(comfirm_title, 2);
		exitLayer->addChild(btm_text, 2);
		addChild(exitLayer, 500);
	}
}

void GameOver::onLeft(Ref *sender)
{
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.ogg");

	getGameLayer()->_isExiting = true;
	Director::sharedDirector()->popScene();
}

void GameOver::onCancel(Ref *sender)
{
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/cancel.ogg");
	exitLayer->removeFromParent();
	exitLayer = nullptr;
}

GameOver *GameOver::create(RenderTexture *snapshoot)
{
	GameOver *pl = new GameOver();
	if (pl && pl->init(snapshoot))
	{
		pl->autorelease();
		return pl;
	}
	else
	{
		delete pl;
		return nullptr;
	}
}
