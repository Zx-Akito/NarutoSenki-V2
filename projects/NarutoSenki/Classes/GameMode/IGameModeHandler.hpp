#pragma once
#include "Core/Hero.hpp"
#include "GameLayer.h"
#include "LoadLayer.h"
#include "SelectLayer.h"
#include <cstring>
#include <sstream>

class IGameModeHandler;
extern "C" int GetNetworkForcedTeam();
extern "C" const char *GetNetworkYourHeroName();
extern "C" const char *GetNetworkEnemyHeroName();
extern "C" const char *GetNetworkYourTeamCsv();
extern "C" const char *GetNetworkEnemyTeamCsv();

enum GameMode : uint32_t
{
	// 1 VS 1 // TODO: As a test mode
	OneVsOne,
	// 3 VS 3
	Classic,
	// 4 VS 4
	FourVsFour,
	// 4 VS 4 (Disabled gears)
	HardCore_4Vs4,
	// 3 VS 1
	Boss,
	// 3 VS 3
	Clone,
	// TODO：3 VS 3
	Deathmatch,
	// 3 VS 3
	RandomDeathmatch,

	__Internal_Max_Length
}; // namespace GameMode

struct GameModeData
{
public:
	string title;
	string description;

	bool isLocked;
	bool useMask2;

	bool hasSelected;
	// IGameModeHandler *handler;

	// static const GameModeData &from(const char *path);
};

struct GameData
{
	bool enableGear = true;
	bool isHardCore = true;
	bool isRandomChar = false;

	bool use4v4SpawnLayout = false;

	Group playerGroup;
};

class IGameModeHandler
{
	friend class GameLayer;
	friend class GameModeLayer;
	friend class SelectLayer;

private:
	vector<HeroData> heroDataVector;
	// map
	int mapId = 0;
	// tower
	bool enableKonohaTowers = true;
	bool enableAkatsukiTowers = true;
	// flog
	bool skipInitFlogs = false;
	bool enableKonohaFlogs = false;
	bool enableAkatsukiFlogs = false;
	float flogSpawnDuration = 15.0f;
	bool isLimitFlog = false;
	int maxFlogWaves;
	// hero
	bool enableHeroReborn = true;

	void Internal_GameOver()
	{
		onGameOver();

		gd = {};
		clearHeroArray();
		resetCheats();
	}

protected:
	SelectLayer *selectLayer = nullptr;
	// game
	GameData gd;
	int oldCheats = -1;
	vector<string> heroVector;
	// player
	Group playerGroup;

public:
	const uint8_t kMaxCharCount = 4;
	const int kDefaultMap = 1;
	const Group kDefaultGroup = Group::Konoha;

	virtual void init() = 0;

	virtual void onInitHeros() = 0;
	virtual void onGameStart() = 0;
	virtual void onGameOver() = 0;

	// callbacks
	virtual void onCharacterInit(CharacterBase *c) = 0;
	virtual void onCharacterDead(CharacterBase *c) = 0;
	virtual void onCharacterReborn(CharacterBase *c) = 0;

	inline const GameData &getGameData() { return gd; }
	inline int getOldCheats() { return oldCheats; }
	inline void setOldCheats(int val) { oldCheats = val; }
	inline void resetCheats()
	{
		if (oldCheats != -1)
		{
			Cheats = oldCheats;
			oldCheats = -1;
		}
	}
	inline const vector<HeroData> &getHerosArray() { return heroDataVector; }

protected:
	// IGameModeHandler()
	// {
	// 	oldCheats = Cheats;
	// }

	void clearHeroArray()
	{
		heroDataVector.clear();
		heroVector.clear();
	}

	// Warpper of game layer
	void setMap(int id)
	{
		this->mapId = id;
	}

	void setTower(bool enableKonohaTowers, bool enableAkatsukiTowers)
	{
		this->enableKonohaTowers = enableKonohaTowers;
		this->enableAkatsukiTowers = enableAkatsukiTowers;
	}

	void setFlog(bool enableKonohaFlogs, bool enableAkatsukiFlogs, float duration = 15.0f, int maxWaves = -1)
	{
		this->enableKonohaFlogs = enableKonohaFlogs;
		this->enableAkatsukiFlogs = enableAkatsukiFlogs;
		this->flogSpawnDuration = (duration < 5 || duration > 60) ? 15.0f : duration;
		this->isLimitFlog = maxWaves >= 0;
		this->maxFlogWaves = maxWaves;
	}

	void setHero(bool enableReborn)
	{
		this->enableHeroReborn = enableReborn;
	}

	// init hero
	inline void addHero(const char *name, Role role, Group group, uint32_t lv = 1)
	{
		heroDataVector.push_back({name, role, group});
		heroVector.push_back(name);
	}

	inline void addHeros(int count, const char *name, Role role, Group group, uint32_t lv = 1)
	{
		for (int i = 0; i < count; i++)
			addHero(name, role, group, lv);
	}

	inline void addKonohaHero(const vector<string> &heros, const char *name, Role role, uint32_t lv = 1)
	{
		addHero(name, role, Group::Konoha, lv);
	}

	inline void addAkatsukiHero(const vector<string> &heros, const char *name, Role role, uint32_t lv = 1)
	{
		addHero(name, role, Group::Akatsuki, lv);
	}

	inline void initHeros(uint32_t playerCount, uint32_t enemyCount)
	{
		initHeros(playerCount, enemyCount, nullptr, kDefaultGroup);
	}

	void initHeros(uint8_t playerCount, uint8_t enemyCount,
				   const char *playerSelect, Group playerGroup,
				   const char *com1Select = nullptr, const char *com2Select = nullptr, const char *com3Select = nullptr)
	{
		clearHeroArray();

		playerCount = playerCount > kMaxCharCount ? kMaxCharCount : playerCount;
		enemyCount = enemyCount > kMaxCharCount ? kMaxCharCount : enemyCount;
		setRand();
		int forcedTeam = GetNetworkForcedTeam();
		int team = forcedTeam >= 0 ? forcedTeam : random(2);
		// TODO: Support custom player group
		// playerGroup = playerGroup == nullptr ? (team > 0 ? Group::Konoha : Group::Akatsuki) : playerGroup;
		playerGroup = team > 0 ? Group::Akatsuki : Group::Konoha;
		this->gd.playerGroup = playerGroup;
		this->playerGroup = playerGroup;
		uint8_t totalCount = playerCount + enemyCount;
		uint8_t comCount = totalCount > 0 ? totalCount - 1 : 0;
		uint8_t comOfPlayerGroupCount = playerCount > 0 ? playerCount - 1 : 0;
		const char *forcedYourHero = GetNetworkYourHeroName();
		const char *forcedEnemyHero = GetNetworkEnemyHeroName();
		const char *forcedYourTeamCsv = GetNetworkYourTeamCsv();
		const char *forcedEnemyTeamCsv = GetNetworkEnemyTeamCsv();
		const bool hasForcedNetworkRoster =
			forcedYourHero && forcedEnemyHero &&
			strlen(forcedYourHero) > 0 && strlen(forcedEnemyHero) > 0;

		if (hasForcedNetworkRoster)
		{
			auto parseCsv = [](const char *csv) {
				vector<string> out;
				if (!csv || strlen(csv) == 0)
					return out;
				std::stringstream ss(csv);
				string item;
				while (std::getline(ss, item, ','))
				{
					if (!item.empty())
						out.push_back(item);
				}
				return out;
			};
			auto yourTeam = parseCsv(forcedYourTeamCsv);
			auto enemyTeam = parseCsv(forcedEnemyTeamCsv);
			// Hard-lock network path to 1v1 roster profile (player + 2 com per side max).
			const uint8_t forcedYourCount = (uint8_t)MIN((int)kMaxCharCount, MAX(1, (int)yourTeam.size()));
			const uint8_t forcedEnemyCount = (uint8_t)MIN((int)kMaxCharCount, MAX(1, (int)enemyTeam.size()));
			playerCount = forcedYourCount;
			enemyCount = forcedEnemyCount;
			this->gd.use4v4SpawnLayout = false;
			// 1v1 network baseline: disable lane minion waves to avoid unsynced AI drift.
			this->skipInitFlogs = true;
			if (!yourTeam.empty())
				forcedYourHero = yourTeam.front().c_str();
			if (!enemyTeam.empty())
				forcedEnemyHero = enemyTeam.front().c_str();

			selectLayer->_playerSelect = forcedYourHero;
			selectLayer->_com1Select = nullptr;
			selectLayer->_com2Select = nullptr;
			selectLayer->_com3Select = nullptr;

			heroDataVector.push_back({forcedYourHero, Role::Player, playerGroup});
			heroVector.push_back(forcedYourHero);

			const Group enemyGroup = playerGroup == Group::Konoha ? Group::Akatsuki : Group::Konoha;
			for (uint8_t i = 1; i < playerCount; i++)
			{
				string hero = i < yourTeam.size() ? yourTeam[i] : getRandomHeroExceptAll(heroVector);
				heroDataVector.push_back({hero, Role::Com, playerGroup});
				heroVector.push_back(hero);
			}

			for (uint8_t i = 0; i < enemyCount; i++)
			{
				string hero;
				if (i < enemyTeam.size())
					hero = enemyTeam[i];
				else if (i == 0)
					hero = forcedEnemyHero;
				else
					hero = getRandomHeroExceptAll(heroVector);
				heroDataVector.push_back({hero, Role::Com, enemyGroup});
				heroVector.push_back(hero);
			}
			return;
		}

		if (playerSelect)
			selectLayer->_playerSelect = playerSelect;
		if (com1Select)
			selectLayer->_com1Select = com1Select;
		if (com2Select)
			selectLayer->_com2Select = com2Select;
		if (com3Select)
			selectLayer->_com3Select = com3Select;

		// init player hero
		string tmpChar;
		if (selectLayer->_playerSelect)
		{
			tmpChar = selectLayer->_playerSelect;
		}
		else
		{
			auto selcetList = selectLayer->_selectList;
			int count = selcetList->count();
			setRand();
			int index = random(count);
			tmpChar = ((CCString *)selcetList->objectAtIndex(index))->m_sString;
			do
			{
				setRand();
				index = random(count);
				tmpChar = ((CCString *)selcetList->objectAtIndex(index))->m_sString;
			} while (tmpChar == "None");

			selectLayer->_playerSelect = tmpChar.c_str();
			gd.isRandomChar = true;
		}

		// push player
		heroDataVector.push_back({tmpChar, Role::Player, playerGroup});
		heroVector.push_back(tmpChar);

		// init com heros
		vector<string> realHeroVector;
		for (size_t i = 0; i < kHeroNum; i++)
		{
			if (is_same(selectLayer->_playerSelect, kHeroList[i]))
				continue;
			if (selectLayer->_com1Select && is_same(selectLayer->_com1Select, kHeroList[i]))
				continue;
			if (selectLayer->_com2Select && is_same(selectLayer->_com2Select, kHeroList[i]))
				continue;
			if (selectLayer->_com3Select && is_same(selectLayer->_com3Select, kHeroList[i]))
				continue;

			realHeroVector.push_back(kHeroList[i]);
		}

		string hero;
		for (int i = 0; i < comCount; i++)
		{
			if (i < comOfPlayerGroupCount)
			{
				if (i == 0 && selectLayer->_com1Select)
				{
					hero = selectLayer->_com1Select;
				}
				else if (i == 1 && selectLayer->_com2Select)
				{
					hero = selectLayer->_com2Select;
				}
				else if (i == 2 && selectLayer->_com3Select)
				{
					hero = selectLayer->_com3Select;
				}
				else
				{
					int count = realHeroVector.size();
					setRand();
					int index = random(count);
					if (index == count)
						index = realHeroVector.size() - 1;

					hero = realHeroVector.at(index);
					realHeroVector.erase(std::find(realHeroVector.begin(), realHeroVector.end(), hero));
				}

				heroDataVector.push_back({hero, Role::Com, team > 0 ? Group::Akatsuki : Group::Konoha});
				heroVector.push_back(hero);
			}
			else
			{
				int count = realHeroVector.size();
				setRand();
				int index = random(count);
				if (index == count)
					index = realHeroVector.size() - 1;

				hero = realHeroVector.at(index);
				heroDataVector.push_back({hero, Role::Com, team > 0 ? Group::Konoha : Group::Akatsuki});
				heroVector.push_back(hero);
				realHeroVector.erase(std::find(realHeroVector.begin(), realHeroVector.end(), hero));
			}
		}
	}

	inline void setPlayerTeamByGroup(Group group)
	{
		gd.playerGroup = group;
	}

	/**
	 * Static Utils
	 */

	static inline const char *getRandomHero()
	{
		setRand();
		return kHeroList[random(kHeroNum)];
	}

	static inline const char *getRandomHeroExcept(const char *except)
	{
		if (except)
		{
			setRand();
			int i = random(kHeroNum);
			while (is_same(kHeroList[i], except))
				i = random(kHeroNum);
			return kHeroList[i];
		}
		else
		{
			return kHeroList[random(kHeroNum)];
		}
	}

	static inline const char *getRandomHeroExceptAll(const vector<string> &excepts, const char *defaultChar = "Naruto")
	{
		auto _begin = excepts.begin();
		auto _end = excepts.end();
		int idx;
		for (size_t i = 0; i < kHeroNum * 2; i++) // Max loops is kHeroNum * 2
		{
			setRand();
			idx = random(kHeroNum);
			if (std::find(_begin, _end, kHeroList[idx]) == _end)
				return kHeroList[idx];
		}
		return defaultChar;
	}

	inline const char *getSelectedOrRandomHero()
	{
		return selectLayer->_playerSelect ? selectLayer->_playerSelect : getRandomHero();
	}

	inline const char *getSelectOrRandomHeroExcept(const char *except)
	{
		return selectLayer->_playerSelect ? selectLayer->_playerSelect : getRandomHeroExcept(except);
	}

	inline const char *getSelectOrRandomHeroExceptAll(const vector<string> &excepts, const char *defaultChar = "Naruto")
	{
		return selectLayer->_playerSelect ? selectLayer->_playerSelect : getRandomHeroExceptAll(excepts, defaultChar);
	}

	static inline Group getRandomGroup()
	{
		setRand();
		return random(2) == 0 ? Group::Konoha : Group::Akatsuki;
	}

	static inline void getRandomGroups(Group &groupA, Group &groupB)
	{
		setRand();
		int team = random(2) == 0;
		groupA = team == 0 ? Group::Konoha : Group::Akatsuki;
		groupB = team == 0 ? Group::Akatsuki : Group::Konoha;
	}
};
