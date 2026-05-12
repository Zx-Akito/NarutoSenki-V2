#include "GameLoginHttp.h"

#include <cctype>
#include <sstream>

#include "cocos2d.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
#include "../../../cocos2dx/platform/third_party/ios/curl/curl.h"
#endif

namespace
{

static std::string trimCopy(const std::string &s)
{
	size_t i = 0;
	while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
	{
		++i;
	}
	size_t j = s.size();
	while (j > i && std::isspace(static_cast<unsigned char>(s[j - 1])))
	{
		--j;
	}
	return s.substr(i, j - i);
}

static bool extractJsonStringValue(const std::string &json, const char *key, std::string *out)
{
	// Tolerant of spaces: "error" : "message"
	const std::string quotedKey = std::string("\"") + key + "\"";
	size_t p = json.find(quotedKey);
	if (p == std::string::npos)
	{
		return false;
	}
	p += quotedKey.size();
	while (p < json.size() && std::isspace(static_cast<unsigned char>(json[p])))
	{
		++p;
	}
	if (p >= json.size() || json[p] != ':')
	{
		return false;
	}
	++p;
	while (p < json.size() && std::isspace(static_cast<unsigned char>(json[p])))
	{
		++p;
	}
	if (p >= json.size() || json[p] != '"')
	{
		return false;
	}
	++p;
	const size_t start = p;
	while (p < json.size())
	{
		if (json[p] == '\\' && p + 1 < json.size())
		{
			p += 2;
			continue;
		}
		if (json[p] == '"')
		{
			*out = json.substr(start, p - start);
			return true;
		}
		++p;
	}
	return false;
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)

static size_t curlWriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp)
{
	userp->append(static_cast<char *>(contents), size * nmemb);
	return size * nmemb;
}

#endif

} // namespace

static std::string jsonEscape(const std::string &s)
{
	std::string o;
	o.reserve(s.size() + 4);
	for (unsigned char uc : s)
	{
		const char c = static_cast<char>(uc);
		if (c == '"' || c == '\\')
		{
			o += '\\';
		}
		o += c;
	}
	return o;
}

GameLoginResult GameLoginHttp::postLogin(const std::string &apiBase, const std::string &usernameOrEmail,
	const std::string &password, const std::string &apiKey)
{
	GameLoginResult r;

#if !(CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	r.errorMessage = "Login HTTP is only enabled on Mac and iOS builds.";
	return r;
#else
	std::string base = trimCopy(apiBase);
	if (base.empty())
	{
		r.errorMessage = "Missing API base URL.";
		return r;
	}

	const std::string url = base + "/api/auth/login";

	CURL *curl = curl_easy_init();
	if (!curl)
	{
		r.errorMessage = "Network init failed.";
		return r;
	}

	std::string responseBody;
	struct curl_slist *headers = nullptr;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	std::string keyHeader;
	if (!trimCopy(apiKey).empty())
	{
		keyHeader = std::string("X-Game-Login-Key: ") + trimCopy(apiKey);
		headers = curl_slist_append(headers, keyHeader.c_str());
	}

	std::ostringstream payload;
	payload << "{\"identifier\":";
	// Minimal JSON string escape for " and \ in identifier/password
	payload << '"' << jsonEscape(usernameOrEmail) << "\",\"password\":\"" << jsonEscape(password) << "\"}";

	const std::string bodyStr = payload.str();

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(bodyStr.size()));
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

	const CURLcode res = curl_easy_perform(curl);
	long httpCode = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK)
	{
		r.errorMessage = curl_easy_strerror(res);
		return r;
	}

	if (httpCode != 200)
	{
		std::string err;
		if (extractJsonStringValue(responseBody, "error", &err))
		{
			r.errorMessage = err;
		}
		else
		{
			std::ostringstream oss;
			oss << "HTTP " << httpCode;
			if (httpCode == 401 || httpCode == 403)
			{
				oss << " (wrong password, API key, or account)";
			}
			else if (httpCode >= 500)
			{
				oss << " (server error — check web deploy / Convex env)";
			}
			else
			{
				oss << " (unexpected response)";
			}
			r.errorMessage = oss.str();
		}
		CCLOG("Login HTTP %ld body: %.300s", httpCode, responseBody.c_str());
		return r;
	}

	if (!extractJsonStringValue(responseBody, "id", &r.id) || !extractJsonStringValue(responseBody, "point", &r.point)
		|| !extractJsonStringValue(responseBody, "group_id", &r.groupId))
	{
		r.errorMessage = "Unexpected server response.";
		return r;
	}

	r.ok = true;
	return r;
#endif
}
