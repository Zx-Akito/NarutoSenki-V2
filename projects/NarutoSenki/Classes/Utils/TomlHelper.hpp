#pragma once
#include "../../../../external/toml.hpp"

// toml extension

#define TOMLEX_NS_BEGIN \
	namespace tomlex    \
	{
#define TOMLEX_NS_END }

TOMLEX_NS_BEGIN

inline std::string trim(const std::string &s)
{
   auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c){return std::isspace(c);});
   auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](int c){return std::isspace(c);}).base();
   return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
}

template <typename T>
inline T get(const toml::table::const_iterator &v)
{
	return toml::get<T>(v->second);
}

// key = 'value'
template <typename T>
inline bool try_set(const toml::table &tab, const toml::key &ky, T &v)
{
	const auto end = tab.end();

	if (const auto iter = tab.find(ky); iter != end)
	{
		v = get<T>(iter->second);
		return true;
	}
	return false;
}

template <typename T>
inline bool try_set_or(const toml::table &tab, const toml::key &ky, T &v, const T &opt)
{
	const auto end = tab.end();

	if (const auto iter = tab.find(ky); iter != end)
	{
		v = get<T>(iter->second);
		return true;
	}
	else
	{
		v = opt;
		return false;
	}
}

TOMLEX_NS_END

#undef TOMLEX_NS_BEGIN
#undef TOMLEX_NS_END
