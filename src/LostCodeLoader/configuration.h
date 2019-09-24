#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>

struct comp
{
	bool operator()(const std::string& lhs, const std::string& rhs) const
	{
		return _stricmp(lhs.c_str(), rhs.c_str()) == 0;
	}
};

struct hash
{
	size_t operator()(const std::string& key) const
	{
		std::string keyCopy(key.size(), ' ');
		std::transform(key.begin(), key.end(), keyCopy.begin(), tolower);
		return std::hash<std::string>()(keyCopy);
	}
};

class ConfigurationFile
{
protected:
	std::unordered_map<std::string, std::unordered_map<std::string, std::string, hash, comp>, hash, comp> sections;
public:
	std::string getString(std::string section, std::string key, const std::string& fallback = std::string());
	int getInt(std::string section, std::string key, int fallback = 0);
	bool getBool(std::string section, std::string key, bool fallback = false);

	void clear();
	void clear(const std::string& section);
	void set(const std::string& section, const std::string& key, const std::string& value);

	static bool open(const std::string& filePath, ConfigurationFile* configuration);
	void save(const std::string& path);

	ConfigurationFile();
};
