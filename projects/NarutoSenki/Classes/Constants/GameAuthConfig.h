#pragma once

/**
 * Base URL of narutosenki-web (no trailing slash).
 * Runtime override: UserDefault "ns_login_api_base" (e.g. http://127.0.0.1:3300 for local npm run dev).
 * Build override: -DNS_GAME_LOGIN_API_BASE_DEFAULT=\"http://127.0.0.1:3300\"
 */
#ifndef NS_GAME_LOGIN_API_BASE_DEFAULT
#define NS_GAME_LOGIN_API_BASE_DEFAULT "http://127.0.0.1:3300"
#endif
