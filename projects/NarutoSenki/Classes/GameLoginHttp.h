#pragma once

#include <string>

struct GameLoginResult
{
	bool ok = false;
	std::string id;
	std::string point;
	std::string coin;
	std::string groupId;
	std::string errorMessage;
};

struct GameSyncCoinResult
{
	bool ok = false;
	std::string coin;
	std::string errorMessage;
};

namespace GameLoginHttp
{
/**
 * POST /api/auth/login on the web app (Convex-backed).
 * @param apiBase e.g. http://127.0.0.1:3300 (no trailing slash)
 * @param apiKey optional; sent as X-Game-Login-Key when non-empty (must match server GAME_LOGIN_API_KEY)
 */
GameLoginResult postLogin(const std::string &apiBase, const std::string &usernameOrEmail, const std::string &password,
	const std::string &apiKey);

/**
 * POST /api/auth/sync-coin — writes SQLite coin total to Convex `users.coin` (same auth as login). Does not touch `point`.
 */
GameSyncCoinResult postSyncCoin(const std::string &apiBase, const std::string &usernameOrEmail,
	const std::string &password, uint32_t sqliteCoin, const std::string &apiKey);
} // namespace GameLoginHttp
