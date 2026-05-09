#include "Core/Hero.hpp"
#include "HudLayer.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
extern "C" bool MacWsIsConnected();
extern "C" int GetNetworkForcedTeam();
#endif

bool HPBar::init(const char *szImage)
{
	RETURN_FALSE_IF(!Sprite::init());

	char fileName[3] = "xx";
	strncpy(fileName, szImage, 2);

	hpBar = Sprite::createWithSpriteFrameName(szImage);
	hpBar->setAnchorPoint(Vec2(0, 0));
	addChild(hpBar, 1);

	if (is_same(fileName, "hp"))
	{
		hpBottom = Sprite::createWithSpriteFrameName("hp_bottom.png");
		hpBar->setPosition(Vec2(15, 1));
	}
	else
	{
		hpBottom = Sprite::createWithSpriteFrameName("flog_bar_buttom.png");
		hpBar->setPosition(Vec2(1, 1));
	}
	hpBottom->setAnchorPoint(Vec2(0, 0));
	addChild(hpBottom, -1);

	return true;
}

void HPBar::changeBar(const char *szImage)
{
	auto frame = getSpriteFrame(szImage);
	hpBar->setDisplayFrame(frame);
}

void HPBar::syncVisualPercent(float hpPercentRemaining)
{
	if (!hpBar)
		return;
	const float p = hpPercentRemaining <= 0.f ? 0.f : hpPercentRemaining;
	auto s = ScaleTo::create(0.2f, p, 1.f);
	hpBar->runAction(s);
}

void HPBar::loseHP(float percent)
{
	if (getGameLayer()->_isHardCoreGame)
	{
		auto gardTower = getGameLayer()->playerGroup == Group::Konoha
							 ? TowerEnum::AkatsukiCenter
							 : TowerEnum::KonohaCenter;
		if (_delegate->getName() == gardTower)
		{
			if (not getGameLayer()->_hasSpawnedGuardian && percent <= 0.8)
			{
				getGameLayer()->initGard();
			}
		}

		if (getGameLayer()->_hasSpawnedGuardian)
		{
			auto _slayer = _delegate->_slayer;
			if (_slayer)
			{
				if (_delegate->getName() == gardTower)
					_slayer->isHurtingTower = true;
				else
					_slayer->isHurtingTower = false;
			}
		}
	}

	if (percent <= 0)
	{
		CharacterBase *_slayer = _delegate->_slayer;
		CharacterBase *currentSlayer = nullptr;

		if (_delegate->isFlog())
		{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
			const bool skipOnlineFollowerLaneRewards =
				MacWsIsConnected() && GetNetworkForcedTeam() != 0;
#else
			const bool skipOnlineFollowerLaneRewards = false;
#endif
			if (_slayer && !skipOnlineFollowerLaneRewards)
			{
				if (_slayer->getSecMaster() &&
					_slayer->getName() != SkillEnum::KageHand &&
					_slayer->getName() != SkillEnum::KageHands)
				{
					if (_slayer->getSecMaster()->getController())
						currentSlayer = _slayer->getSecMaster()->getController();
					else
						currentSlayer = _slayer->getSecMaster();
				}
				else if (_slayer->getMaster())
				{
					if (_slayer->getMaster()->getController())
						currentSlayer = _slayer->getMaster()->getController();
					else
						currentSlayer = _slayer->getMaster();
				}
				else
				{
					if (_slayer->getController())
						currentSlayer = _slayer->getController();
					else
						currentSlayer = _slayer;
				}

				if (currentSlayer->getLV() != 6)
				{
					if (currentSlayer->getName() == HeroEnum::Naruto ||
						currentSlayer->getName() == HeroEnum::SageNaruto)
					{
						currentSlayer->setEXP(currentSlayer->getEXP() + 12);
						currentSlayer->changeHPbar();
					}
					else
					{
						currentSlayer->setEXP(currentSlayer->getEXP() + 10);
						currentSlayer->changeHPbar();
					}
				}

				if (currentSlayer->isPlayer())
				{
					if (_delegate->getMaxHP() == 10000)
					{
						_delegate->setCoinDisplay(30);
						getGameLayer()->setCoin("30");
						getGameLayer()->getHudLayer()->setEXPLose();
					}
					else if (_delegate->getMaxHP() == 5000)
					{
						_delegate->setCoinDisplay(20);
						getGameLayer()->setCoin("20");
						getGameLayer()->getHudLayer()->setEXPLose();
					}
					else
					{
						_delegate->setCoinDisplay(10);
						getGameLayer()->setCoin("10");
						getGameLayer()->getHudLayer()->setEXPLose();
					}
				}

				currentSlayer->_flogNum += 1;
				if (_delegate->getMaxHP() == 10000)
					currentSlayer->addCoin(30);
				else if (_delegate->getMaxHP() == 5000)
					currentSlayer->addCoin(20);
				else
					currentSlayer->addCoin(10);
			}
		}
		else if (_delegate->isTower())
		{
			if (_slayer)
			{
				if (_slayer->getSecMaster() &&
					_slayer->getName() != SkillEnum::KageHand &&
					_slayer->getName() != SkillEnum::KageHands &&
					_slayer->getName() != SummonEnum::SmallSlug)
				{
					if (_slayer->getSecMaster()->getController())
						currentSlayer = _slayer->getSecMaster()->getController();
					else
						currentSlayer = _slayer->getSecMaster();
				}
				else if (_slayer->getMaster())
				{
					if (_slayer->getMaster()->getController())
						currentSlayer = _slayer->getMaster()->getController();
					else
						currentSlayer = _slayer->getMaster();
				}
				else
				{
					if (_slayer->getController())
						currentSlayer = _slayer->getController();
					else
						currentSlayer = _slayer;
				}

				if ((currentSlayer->isNotFlog()))
				{
					getGameLayer()->setReport(currentSlayer->getName(), kRoleTower, currentSlayer->getKillNum());

					if (currentSlayer->getLV() != 6)
					{
						if (currentSlayer->getName() == HeroEnum::Naruto ||
							currentSlayer->getName() == HeroEnum::SageNaruto)
						{
							currentSlayer->setEXP(currentSlayer->getEXP() + 625);
							currentSlayer->changeHPbar();
						}
						else
						{
							currentSlayer->setEXP(currentSlayer->getEXP() + 500);
							currentSlayer->changeHPbar();
						}
					}

					if (currentSlayer->isPlayer())
					{
						if (getGameLayer()->_isHardCoreGame)
						{
							if (_delegate->getMaxHP() > 40000)
							{
								getGameLayer()->setCoin("1000");
								_delegate->setCoinDisplay(1000);
							}
							else
							{
								getGameLayer()->setCoin("500");
								_delegate->setCoinDisplay(500);
							}
						}
						else
						{
							getGameLayer()->setCoin("300");
							_delegate->setCoinDisplay(300);
						}

						getGameLayer()->getHudLayer()->setEXPLose();
					}

					if (getGameLayer()->_isHardCoreGame)
					{
						if (_delegate->getMaxHP() > 40000)
							currentSlayer->addCoin(1000);
						else
							currentSlayer->addCoin(500);
					}
					else
					{
						currentSlayer->addCoin(300);
					}
				}

				for (auto otherSlayer : getGameLayer()->_CharacterArray)
				{
					if (currentSlayer->getGroup() == otherSlayer->getGroup() &&
						currentSlayer->getName() != otherSlayer->getName())
					{
						if (otherSlayer->getLV() != 6)
						{
							otherSlayer->setEXP(otherSlayer->getEXP() + 250);
							otherSlayer->changeHPbar();
						}

						if (otherSlayer->isPlayer())
						{
							if (getGameLayer()->_isHardCoreGame)
							{
								if (_delegate->getMaxHP() > 40000)
								{
									getGameLayer()->setCoin("850");
									_delegate->setCoinDisplay(850);
								}
								else
								{
									getGameLayer()->setCoin("350");
									_delegate->setCoinDisplay(350);
								}
							}
							else
							{
								getGameLayer()->setCoin("150");
								_delegate->setCoinDisplay(150);
							}

							getGameLayer()->getHudLayer()->setEXPLose();
						}

						if (getGameLayer()->_isHardCoreGame)
						{
							if (_delegate->getMaxHP() > 40000)
								otherSlayer->addCoin(850);
							else
								otherSlayer->addCoin(350);
						}
						else
						{
							otherSlayer->addCoin(150);
						}
					}
				}
			}
		}
		else if (_delegate->isPlayerOrCom())
		{
			if (_delegate->getName() == HeroEnum::Kakuzu && getGameLayer()->_isOugis2Game)
			{
				bool reieveAble = false;
				if (_delegate->getCKR2() >= 25000 && _delegate->hearts > 0)
				{
					if (_delegate->isPlayer())
						reieveAble = true;
					else if (_delegate->isCom())
						reieveAble = true;
				}

				if (reieveAble && _delegate->getState() != State::O2ATTACK && !_delegate->_isInvincible && _delegate->getState() != State::DEAD)
				{
					if (_delegate->_isSticking)
						_delegate->_isSticking = false;

					if (_delegate->getState() == State::FLOAT ||
						_delegate->getState() == State::AIRHURT)
					{
						setPositionY(_delegate->_originY);
						_delegate->_originY = 0;
						getGameLayer()->reorderChild(_delegate, -_delegate->getPositionY());
					}

					if (_delegate->isPlayer())
					{
						getGameLayer()->getHudLayer()->skill5Button->unLock();
						_delegate->setState(State::IDLE);
						getGameLayer()->setSkillFinish(true);
						getGameLayer()->getHudLayer()->skill5Button->click();
						getGameLayer()->getHudLayer()->skill5Button->setLock();
					}
					else
					{
						if (_delegate->_isCanOugis2)
						{
							_delegate->setState(State::IDLE);
							_delegate->attack(OUGIS2);
						}
					}
					_delegate->hearts -= 1;

					if (_delegate->_heartEffect)
					{
						auto frame = getSpriteFrame("Heart_Effect_{:02d}", _delegate->hearts);
						_delegate->_heartEffect->setDisplayFrame(frame);
					}

					if (_delegate->hearts < 1)
					{
						if (_delegate->isPlayer())
							getGameLayer()->getHudLayer()->skill4Button->setLock();
					}

					return;
				}

				if (_delegate->getState() == State::O2ATTACK)
				{
					return;
				}
			}

			if (_delegate->_isControlled)
			{
				_delegate->_isSuicide = true;
				currentSlayer = _delegate->getController();
			}
			else if (_slayer)
			{
				if (_slayer->getSecMaster() &&
					_slayer->getName() != SkillEnum::KageHand &&
					_slayer->getName() != SkillEnum::KageHands &&
					_slayer->getName() != SummonEnum::SmallSlug &&
					_slayer->getName() != SkillEnum::FakeItachi)
				{
					if (_slayer->getSecMaster()->getController())
						currentSlayer = _slayer->getSecMaster()->getController();
					else
						currentSlayer = _slayer->getSecMaster();
				}
				else if (_slayer->getMaster())
				{
					if (_slayer->getMaster()->getController())
						currentSlayer = _slayer->getMaster()->getController();
					else
						currentSlayer = _slayer->getMaster();
				}
				else
				{
					if (_slayer->getController())
						currentSlayer = _slayer->getController();
					else
						currentSlayer = _slayer;
				}
			}

			if (currentSlayer)
			{
				if (currentSlayer->getName() == HeroEnum::Kakuzu)
				{
					if (currentSlayer->hearts <= 4)
					{
						currentSlayer->hearts += 1;

						if (currentSlayer->_heartEffect)
						{
							auto frame = getSpriteFrame("Heart_Effect_{:02d}", currentSlayer->hearts);
							currentSlayer->_heartEffect->setDisplayFrame(frame);
						}

						if (currentSlayer->isPlayer())
						{
							if (currentSlayer->getMonsterArray().size() < 3 && currentSlayer->getLV() >= 2)
							{
								getGameLayer()->getHudLayer()->skill4Button->unLock();
							}
						}
					}
				}

				if (currentSlayer->isNotFlog())
				{
					uint32_t realKillNum = currentSlayer->getKillNum() + 1;
					currentSlayer->setKillNum(realKillNum);
					getGameLayer()->setReport(currentSlayer->getName(), _delegate->getName(), currentSlayer->getKillNum());
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
					if (MacWsIsConnected() && currentSlayer == getGameLayer()->currentPlayer)
						getGameLayer()->syncOnlineBattleStatsToPeer(true);
#endif

					auto currentTeam = getGameLayer()->playerGroup;
					if (currentTeam == currentSlayer->getGroup())
					{
						int teamKills = to_int(getGameLayer()->getHudLayer()->KonoLabel->getString()) + 1;
						getGameLayer()->getHudLayer()->KonoLabel->setString(to_cstr(teamKills));
					}
					else
					{
						int teamKills = to_int(getGameLayer()->getHudLayer()->AkaLabel->getString()) + 1;
						getGameLayer()->getHudLayer()->AkaLabel->setString(to_cstr(teamKills));
					}

					if (currentSlayer->isNotGuardian())
					{
						uint32_t newTime = getGameLayer()->getTotalKills() + 1;
						getGameLayer()->setTotalKills(newTime);
					}

					if (currentSlayer->getLV() != 6)
					{
						if (currentSlayer->getName() == HeroEnum::Naruto ||
							currentSlayer->getName() == HeroEnum::SageNaruto)
						{
							currentSlayer->setEXP(currentSlayer->getEXP() + 125);
							currentSlayer->changeHPbar();
						}
						else
						{
							currentSlayer->setEXP(currentSlayer->getEXP() + 100);
							currentSlayer->changeHPbar();
						}
					}

					if (currentSlayer->isPlayer())
					{
						if (_delegate->isGuardian())
						{
							getGameLayer()->setCoin("1000");
							_delegate->setCoinDisplay(1000);
						}
						else
						{
							if (getGameLayer()->_isHardCoreGame)
							{
								getGameLayer()->setCoin(to_cstr(50 + (_delegate->getLV() - 1) * 10));
								_delegate->setCoinDisplay(50 + (_delegate->getLV() - 1) * 10);
							}
							else
							{
								getGameLayer()->setCoin(to_cstr(50));
								_delegate->setCoinDisplay(50);
							}
						}

						getGameLayer()->getHudLayer()->setEXPLose();
						const char *kl = getGameLayer()->getHudLayer()->killLabel->getString();
						int kills = to_int(kl) + 1;
						getGameLayer()->getHudLayer()->killLabel->setString(to_cstr(kills));
					}

					if (_delegate->isGuardian())
					{
						currentSlayer->addCoin(1000);
					}
					else
					{
						if (getGameLayer()->_isHardCoreGame)
							currentSlayer->addCoin(50 + (_delegate->getLV() - 1) * 10);
						else
							currentSlayer->addCoin(50);
					}
				}

				for (auto otherSlayer : getGameLayer()->_CharacterArray)
				{
					if (currentSlayer->getGroup() == otherSlayer->getGroup() &&
						currentSlayer->getName() != otherSlayer->getName())
					{
						if (otherSlayer->getLV() != 6)
						{
							otherSlayer->setEXP(otherSlayer->getEXP() + 25);
							otherSlayer->changeHPbar();
						}

						if (otherSlayer->isPlayer())
						{
							if (getGameLayer()->_isHardCoreGame)
							{
								if (_delegate->isGuardian())
								{
									getGameLayer()->setCoin(to_cstr(850));
									_delegate->setCoinDisplay(850);
								}
								else
								{
									getGameLayer()->setCoin(to_cstr(25 + (_delegate->getLV() - 1) * 10));
									_delegate->setCoinDisplay(25 + (_delegate->getLV() - 1) * 10);
								}
							}
							else
							{
								getGameLayer()->setCoin(to_cstr(25));
								_delegate->setCoinDisplay(25);
							}
							getGameLayer()->getHudLayer()->setEXPLose();
						}
						if (getGameLayer()->_isHardCoreGame)
						{
							if (_delegate->isGuardian())
								otherSlayer->addCoin(850);
							else
								otherSlayer->addCoin(25 + (_delegate->getLV() - 1) * 10);
						}
						else
						{
							otherSlayer->addCoin(25);
						}
					}
				}
			}
		}

		_delegate->dead();
	}
	else
	{
		auto s = ScaleTo::create(0.2f, percent, 1);
		hpBar->runAction(s);

		if (_delegate->isPlayer())
			getGameLayer()->setHPLose(percent);
	}
}

HPBar *HPBar::create(const char *szImage)
{
	HPBar *hb = new HPBar();
	if (hb && hb->init(szImage))
	{
		hb->autorelease();
		return hb;
	}
	else
	{
		delete hb;
		return nullptr;
	}
}
