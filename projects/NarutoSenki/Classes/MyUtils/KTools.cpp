#include "KTools.h"
#include "MyUtils/CMD5Checksum.h"
#include "Utils/Cocos2dxHelper.hpp"
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include "../../../cocos2dx/platform/android/jni/JniHelper.h"
#endif

bool KTools::readXMLToArray(const string &filePath, CCArray *&array)
{
	unsigned long nSize;
	auto data = (const char *)FileUtils::sharedFileUtils()->getFileData(filePath.c_str(), "r", &nSize);
	if (data == nullptr)
	{
		CCMessageBox(format("Data {} is null", filePath).c_str(), "Read XML Error");
		return false;
	}
	tinyxml2::XMLDocument doc;
	auto err = doc.Parse(data, nSize);
	delete data;
	if (err)
	{
		CCMessageBox(format("XML Error: {}", tinyxml2::XMLDocument::ErrorIDToName(err)).c_str(), "Parse XML Error");
		return false;
	}

	auto rootEle = doc.RootElement();
	auto actionEle = rootEle->FirstChildElement();

	while (actionEle)
	{
		// const char *actionName = actionEle->FirstAttribute()->Value();
		// CCLog("[%s]",actionName);

		auto actionNodeEle = actionEle->FirstChildElement();
		auto actionArray = CCArray::create();
		auto actionData = CCArray::create();
		auto actionFrame = CCArray::create();
		while (actionNodeEle)
		{
			// CCLog("+[%s]",actionNodeEle->Name());
			if (!is_same(actionNodeEle->Name(), "frame"))
			{
				auto dataEle = actionNodeEle->FirstChildElement();

				while (dataEle)
				{
					const char *nodeKey = dataEle->FirstAttribute()->Value();
					CCString *nodeValue = CCString::create(dataEle->GetText());

					// CCLog("%s:%s",nodeKey,nodeValue->getCString());
					auto dataDic = CCDictionary::create();
					dataDic->setObject(nodeValue, nodeKey);

					actionData->addObject(dataDic);
					dataEle = dataEle->NextSiblingElement();
				}
			}
			else
			{
				auto frameEle = actionNodeEle->FirstChildElement();

				while (frameEle)
				{
					const char *nodeKey;
					if (is_same(frameEle->Name(), "f"))
						nodeKey = frameEle->Name();
					else
						nodeKey = frameEle->FirstAttribute()->Value();

					CCString *nodeValue = CCString::create(frameEle->GetText());

					// CCLog("%s:%s",nodeKey,nodeValue->getCString());
					auto frameDic = CCDictionary::create();
					frameDic->setObject(nodeValue, nodeKey);
					actionFrame->addObject(frameDic);

					frameEle = frameEle->NextSiblingElement();
				}
			}
			actionNodeEle = actionNodeEle->NextSiblingElement();
		}
		actionArray->addObject(actionData);
		actionArray->addObject(actionFrame);

		array->addObject(actionArray);
		actionEle = actionEle->NextSiblingElement();
	}

	return true;
}

int isExisted(void *para, int n_column, char **column_value, char **column_name)
{
	bool *isExisted_ = (bool *)para;
	*isExisted_ = (**column_value) != '0';
	return 0;
}

void KTools::encode(string &str, int radomKey)
{
	for (size_t i = 0; i < str.length(); i++)
	{
		int ch = str[i];
		ch += radomKey;
		str[i] = ch;
	}
	str += radomKey;
}

void KTools::decode(string &str)
{
	size_t num = str.length() - 1;
	int key = str[num];
	str = str.substr(0, num);

	for (size_t i = 0; i < str.length(); i++)
	{
		int ch = str[i];
		ch -= key;
		str[i] = ch;
	}
}

void KTools::initTableInDB()
{
	sqlite3 *pDB = nullptr;
	char *errorMsg = nullptr;
	string path = FileUtils::sharedFileUtils()->getWritablePath() + "sql.db";
	int result = sqlite3_open(path.c_str(), &pDB);
	if (result != SQLITE_OK)
	{
		CCLOG("open sql file Failed!");
		return;
	}

	string sql = "drop table IF EXISTS Achievement";

	sqlite3_exec(pDB, sql.c_str(), nullptr, nullptr, &errorMsg);

	sql = "create table if not exists CharRecord (name char(20)  primary key ,column1 char(10),column2 char(10),column3 char(10))";
	sqlite3_exec(pDB, sql.c_str(), nullptr, nullptr, &errorMsg);

	sql = "select * from  GameRecord";

	sqlite3_exec(pDB, sql.c_str(), nullptr, nullptr, &errorMsg);

	if (errorMsg != nullptr)
	{
		errorMsg = nullptr;
		sql = "create table if not exists GameRecord (id char(10) primary key,coin char(20),version char(20))";
		sqlite3_exec(pDB, sql.c_str(), nullptr, nullptr, &errorMsg);

		auto coin = "n>";
		sql = format("insert into GameRecord values(1,'{}','1')", coin);
		int result = sqlite3_exec(pDB, sql.c_str(), nullptr, nullptr, &errorMsg);

		if (errorMsg != nullptr)
		{
			CCLOG("exec sql %s failed with mgs: %s", sql.c_str(), errorMsg);
			sqlite3_close(pDB);
			return;
		}
	}

	bool isExisted_;

	sql = "select count(*) from  CharRecord";
	sqlite3_exec(pDB, sql.c_str(), isExisted, &isExisted_, &errorMsg);

	if (!isExisted_)
	{
		for (size_t i = 0; i < kHeroNum; i++)
		{
			string name = kHeroList[i];
			int key = rand() % 50 + 40;
			encode(name, key);

			string column1DB = "0";
			key = rand() % 60 + 40;
			encode(column1DB, key);

			string column2DB = "0";
			key = rand() % 60 + 40;
			encode(column2DB, key);

			string column3DB = "";
			key = rand() % 60 + 40;
			encode(column3DB, key);

			sql = format("insert into  CharRecord values('{}','{}','{}','{}')", name, column1DB, column2DB, column3DB);
			sqlite3_exec(pDB, sql.c_str(), nullptr, nullptr, &errorMsg);

			if (errorMsg != nullptr)
			{
				CCLOG("exec sql %s failed with mgs: %s", sql.c_str(), errorMsg);
				sqlite3_close(pDB);
				return;
			}
		}
	}

	sqlite3_close(pDB);
}

void KTools::initColumeInDB()
{
	sqlite3 *pDB = nullptr;
	char *errorMsg = nullptr;
	string path = FileUtils::sharedFileUtils()->getWritablePath() + "sql.db";
	int result = sqlite3_open(path.c_str(), &pDB);
	if (result != SQLITE_OK)
	{
		CCLOG("open sql file Failed!");
		return;
	}

	char **result2;
	int row = 0;
	int column = 0;

	string sql = "select coin from GameRecord";
	sqlite3_get_table(pDB, sql.c_str(), &result2, &row, &column, nullptr);
	string str2 = result2[1];
	decode(str2);

	string str3 = "uuuuu<";
	decode(str3);
	if (to_int(str2.c_str()) > to_int(str3.c_str()))
	{
		auto coin = "uuuuu<";
		sql = format("update GameRecord set coin='{}'", coin);
		sqlite3_exec(pDB, sql.c_str(), nullptr, nullptr, &errorMsg);
	}
}

void KTools::prepareFileOGG(const string &listName, bool unload /* =false */)
{
	if (!UserDefault::sharedUserDefault()->getBoolForKey("isPreload"))
	{
		return;
	}

	// MD5
	const string md5Path = "Audio/list.xml";
	bool isExisted = FileUtils::sharedFileUtils()->isFileExist(md5Path);
	if (!isExisted)
	{
		return;
	}

	unsigned long nSize;
	const char *pXmlBuffer = (const char *)FileUtils::sharedFileUtils()->getFileData(md5Path.c_str(), "r", &nSize);
	tinyxml2::XMLDocument doc;
	doc.Parse(pXmlBuffer);
	delete pXmlBuffer;

	auto rootEle = doc.RootElement();
	auto fileEle = rootEle->FirstChildElement();

	while (fileEle)
	{
		auto pathSrc = fileEle->FirstAttribute()->Value();
		auto soundPath = fileEle->GetText();
		if (pathSrc == listName)
		{
			if (!unload)
			{
				SimpleAudioEngine::sharedEngine()->preloadEffect(soundPath);
			}
			else if (unload)
			{
				SimpleAudioEngine::sharedEngine()->unloadEffect(soundPath);
			}
		}

		fileEle = fileEle->NextSiblingElement();
	}
}

sqlite3 *KTools::prepareTableInDB()
{
	sqlite3 *pDB = nullptr;

	string path = FileUtils::sharedFileUtils()->getWritablePath() + "sql.db";
	int result = sqlite3_open(path.c_str(), &pDB);

	if (result != SQLITE_OK)
	{
		CCMessageBox(path.c_str(), "open sql file Failed!");
		return nullptr;
	}

	return pDB;
}

bool KTools::saveToSQLite(const char *table /* ="GameRecord"*/, const char *column /* =nullptr */, const char *value /* =nullptr */, bool isBuy /*= false */)
{
	sqlite3 *pDB = prepareTableInDB();
	char *errorMsg = nullptr;
	if (pDB != nullptr)
	{
		int key = rand() % 60 + 40;
		string coin = value;
		encode(coin, key);

		string sql = format("update {} set {}='{}'", table, column, coin);
		sqlite3_exec(pDB, sql.c_str(), nullptr, nullptr, &errorMsg);

		if (errorMsg != nullptr)
		{
			sqlite3_close(pDB);
			return false;
		}

		return true;
	}

	sqlite3_close(pDB);
	return false;
}

string KTools::readFromSQLite(const char *table /* ="GameRecord" */, const char *column /* =nullptr */, const char *value /* =nullptr */)
{
	sqlite3 *pDB = prepareTableInDB();

	if (pDB != nullptr)
	{
		char **result = nullptr;

		string sql = format("select {} from  GameRecord", "coin");
		int row = 0;
		int column = 0;
		char *errMsg = nullptr;

		const int rc = sqlite3_get_table(pDB, sql.c_str(), &result, &row, &column, &errMsg);
		if (rc != SQLITE_OK || result == nullptr || column < 1 || row < 1)
		{
			if (result)
				sqlite3_free_table(result);
			if (errMsg)
				sqlite3_free(errMsg);
			sqlite3_close(pDB);
			return "";
		}
		const int numCells = column * (row + 1);
		if (numCells <= column)
		{
			sqlite3_free_table(result);
			if (errMsg)
				sqlite3_free(errMsg);
			sqlite3_close(pDB);
			return "";
		}

		string str = result[column];
		decode(str);
		sqlite3_free_table(result);
		sqlite3_close(pDB);
		return str;
	}
	sqlite3_close(pDB);
	return "";
}

string KTools::readSQLite(const char *table, const char *column, const char *value, const char *targetColumn)
{
	sqlite3 *pDB = prepareTableInDB();

	if (pDB != nullptr)
	{
		char **result = nullptr;

		string sql = format("select {},{} from  {}", column, targetColumn, table);
		int row = 0;
		int ncol = 0;
		char *errMsg = nullptr;

		const int rc = sqlite3_get_table(pDB, sql.c_str(), &result, &row, &ncol, &errMsg);
		if (rc != SQLITE_OK || result == nullptr || ncol < 1 || row < 0)
		{
			if (result)
				sqlite3_free_table(result);
			if (errMsg)
				sqlite3_free(errMsg);
			sqlite3_close(pDB);
			return "";
		}
		const int numCells = ncol * (row + 1);
		if (numCells < 2)
		{
			sqlite3_free_table(result);
			if (errMsg)
				sqlite3_free(errMsg);
			sqlite3_close(pDB);
			return "";
		}

		string target;
		for (int i = 0; i + 1 < numCells; i++)
		{
			string str = result[i];

			decode(str);
			if (str == value)
			{
				target = result[i + 1];
				decode(target);
				break;
			}
		}

		if (!is_same(targetColumn, "column3") &&
			!is_same(targetColumn, "column4"))
		{
			target = std::to_string(to_int(target.c_str()));
		}

		sqlite3_free_table(result);
		if (errMsg)
			sqlite3_free(errMsg);
		sqlite3_close(pDB);
		return target;
	}
	sqlite3_close(pDB);
	return "";
}

void KTools::saveSQLite(const char *table, const char *relatedColumn, const char *value, const char *targetColumn, const string &targetValue, bool isPlus)
{
	char *errorMsg = nullptr;
	sqlite3 *pDB = prepareTableInDB();

	if (pDB != nullptr)
	{
		char **result = nullptr;

		string sql = format("select {},{} from {}", relatedColumn, targetColumn, table);
		int row = 0;
		int column = 0;
		char *getErr = nullptr;

		const int rcGet = sqlite3_get_table(pDB, sql.c_str(), &result, &row, &column, &getErr);
		if (rcGet != SQLITE_OK || result == nullptr || column < 1 || row < 0)
		{
			if (result)
				sqlite3_free_table(result);
			if (getErr)
				sqlite3_free(getErr);
			sqlite3_close(pDB);
			return;
		}
		const int numCells = column * (row + 1);
		if (numCells < 2)
		{
			sqlite3_free_table(result);
			if (getErr)
				sqlite3_free(getErr);
			sqlite3_close(pDB);
			return;
		}

		string target;
		string columnValue;
		for (int i = 0; i + 1 < numCells; i++)
		{
			string str = result[i];
			decode(str);
			if (str == value)
			{
				columnValue = result[i];
				target = result[i + 1];
				decode(target);
				break;
			}
		}

		string saveValue;
		if (isPlus)
		{
			saveValue = targetValue + target;
			sql = format(("update {} set {}='{}' where {}='{}'"), table, targetColumn, saveValue, relatedColumn, columnValue);
		}
		else
		{
			saveValue = targetValue;
		}
		int key = rand() % 50 + 40;
		encode(saveValue, key);
		sql = format("update {} set {}='{}' where {}='{}'", table, targetColumn, saveValue, relatedColumn, columnValue);
		sqlite3_exec(pDB, sql.c_str(), nullptr, nullptr, nullptr);

		if (isPlus)
		{
			char **result2 = nullptr;
			sql = "select coin from GameRecord";
			char *errCoin = nullptr;
			const int rcCoin = sqlite3_get_table(pDB, sql.c_str(), &result2, &row, &column, &errCoin);
			if (rcCoin == SQLITE_OK && result2 != nullptr && column >= 1 && row >= 1 &&
				column * (row + 1) > column)
			{
				string str2 = result2[column];
				decode(str2);

				string str3 = "uuuuu<";
				decode(str3);

				if (to_int(str2.c_str()) > to_int(str3.c_str()))
				{
					auto coin = "uuuuu<";
					sql = format("update GameRecord set coin='{}'", coin);
					sqlite3_exec(pDB, sql.c_str(), nullptr, nullptr, &errorMsg);
				}
			}
			if (result2)
				sqlite3_free_table(result2);
			if (errCoin)
				sqlite3_free(errCoin);
		}

		sqlite3_free_table(result);
		if (getErr)
			sqlite3_free(getErr);
		sqlite3_close(pDB);
		return;
	}
	sqlite3_close(pDB);
}

int KTools::readWinNumFromSQL(const char *heroName)
{
	auto winNum = readSQLite("CharRecord", "name", heroName, "column1").c_str();
	return to_int(winNum);
}

int KTools::readLoseNumFromSQL(const char *heroName)
{
	const string loseStr = readSQLite("CharRecord", "name", heroName, "column2");
	if (loseStr.empty())
	{
		return 0;
	}
	return to_int(loseStr.c_str());
}

int KTools::readCoinFromSQL()
{
	auto coins = readFromSQLite();
	return to_int(coins.c_str());
}

const char *KTools::readRecordTimeFromSQL(const char *heroName)
{
	auto recordTime = readSQLite("CharRecord", "name", heroName, "column3").c_str();
	return recordTime;
}

string KTools::encodeData(string data)
{
	// SHA1 *sha1;
	// unsigned char *digest;
	string dataMD5 = CMD5Checksum::GetMD5OfString(data);

	// sha1=new SHA1;
	////sha1->addBytes( "goldlion" ,strlen( "goldlion" ));//
	// string result=(char*) digest;
	// delete sha1;
	// free(digest);
	return dataMD5;
}

bool CCTips::init(const char *tips)
{
	RETURN_FALSE_IF(!Sprite::init());

	setAnchorPoint(Vec2(0.5, 0.5));
	auto strings = CCDictionary::createWithContentsOfFile("Config/strings.xml");
	const char *reply = ((CCString *)strings->objectForKey(tips))->m_sString.c_str();

	CCLabelTTF *tipLabel = CCLabelTTF::create(reply, FONT_NAME, 12);
	addChild(tipLabel, 5000);
	setPosition(Vec2(winSize.width / 2, 50));
	auto call = CallFunc::create(std::bind(&CCTips::onDestroy, this));

	auto mv = MoveBy::create(0.2f, Vec2(0, 12));
	auto fadeOut = FadeOut::create(0.2f);
	auto delay = DelayTime::create(2.0f);
	auto sp = Spawn::create(fadeOut, mv, nullptr);
	auto seq = newSequence(sp, delay, call);
	runAction(seq);

	return true;
}

void CCTips::onDestroy()
{
	removeAllChildren();
	removeFromParent();
}

CCTips *CCTips::create(const char *tips)
{
	CCTips *ab = new CCTips();
	if (ab && ab->init(tips))
	{
		ab->autorelease();
		return ab;
	}
	else
	{
		delete ab;
		return nullptr;
	}
}
