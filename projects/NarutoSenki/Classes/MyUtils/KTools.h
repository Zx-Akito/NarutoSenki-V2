#pragma once
#include "Defines.h"
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include "../../external/sqlite3/src/sqlite3.h"
#else
#include "sqlite3.h"
#endif

class KTools
{
public:
	static bool readXMLToArray(const string &filePath, CCArray *&array);
	static void prepareFileOGG(const string &listName, bool unload = false);

	static void initTableInDB();
	static void initColumeInDB();

	static sqlite3 *prepareTableInDB();

	static bool saveToSQLite(const char *table = "GameRecord", const char *column = nullptr, const char *value = nullptr, bool isBuy = false);
	static string readFromSQLite(const char *table = "GameRecord", const char *column = nullptr, const char *value = nullptr);

	static void encode(string &str, int randomKey);
	static void decode(string &str);

	static string readSQLite(const char *table, const char *column, const char *value, const char *targetColumn);
	static void saveSQLite(const char *table, const char *relatedColumn, const char *value, const char *targetColumn, const string &targetValue, bool isPlus);

	static int readWinNumFromSQL(const char *heroName);
	/** CharRecord.column2 — jumlah kalah per hero (di-update saat game over kalah). */
	static int readLoseNumFromSQL(const char *heroName);
	static int readCoinFromSQL();
	static const char *readRecordTimeFromSQL(const char *heroName);

	static string encodeData(string data);
};

class CCTips : public Sprite
{
public:
	bool init(const char *tips);

	static CCTips *create(const char *tips);

protected:
	void onDestroy();
};
