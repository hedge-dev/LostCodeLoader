#include "pch.h"
#include "configuration.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

#define ASSERT(condition, message) if ( !( condition ) ) { errorMsg = message; goto error; }

static void trim(std::string& str, const std::string& toTrim = " \n\r\t\"\\/")
{
	str.erase(0, str.find_first_not_of(toTrim));
	str.erase(str.find_last_not_of(toTrim) + 1);
}

std::string ConfigurationFile::getString(std::string section, std::string key, const std::string& fallback)
{
	std::string value = sections[section][key];
	return !value.empty() ? value : fallback;
}

int ConfigurationFile::getInt(std::string section, std::string key, int fallback)
{
	std::string value = sections[section][key];
	return !value.empty() ? std::stoi(value) : fallback;
}

bool ConfigurationFile::getBool(std::string section, std::string key, bool fallback)
{
	std::string value = sections[section][key];
	return value != "false" && value != "0";
}

void ConfigurationFile::clear()
{
	sections.clear();
}

void ConfigurationFile::clear(const std::string& section)
{
	sections[section].clear();
}

void ConfigurationFile::set(const std::string& section, const std::string& key, const std::string& value)
{
	auto pair = sections.find(section);
	if (pair == sections.end())
	{
		sections[section] = {{key, value}};
	}
	else
	{
		(*pair).second[key] = value;
	}
}

void ConfigurationFile::save(const std::string& filePath)
{
	std::ofstream stream(filePath, std::ofstream::out);
	if (!stream.is_open())
		return;

	std::string str;
	for (auto& section : sections)
	{
		str.append("[" + section.first + "]\n");
		stream.write(str.c_str(), str.size());

		for (auto& value : section.second)
		{
			if (value.second.empty() || value.second.find_first_not_of("-.0123456789") != std::string::npos ||
				(value.second != "true" && value.second != "false"))
			{
				str.append("\"" + value.second + "\"");
			}
			else
			{
				str.append(value.second);
			}

			str.append(value.first + "=" + value.second + "\n");
			stream.write(str.c_str(), str.size());
		}
	}
	stream.close();
}

ConfigurationFile::ConfigurationFile()
{
}

bool ConfigurationFile::open(const std::string& filePath, ConfigurationFile* configuration)
{
	std::ifstream stream(filePath, std::ifstream::in);
	if (!stream.is_open())
	{
		configuration->sections.clear();
		return false;
	}

	std::string line;
	std::string sectionName;
	std::unordered_map<std::string, std::string, hash, comp> section;

	std::string errorMsg;
	size_t lineIndex = 0;

	while (std::getline(stream, line))
	{
		trim(line);

		if (line.empty() || line[0] == ';' || line[0] == '#')
			continue;

		if (line[0] == '[')
		{
			size_t index = line.find(']');

			ASSERT(index != std::string::npos, "Expected closing bracket after section name");

			if (!sectionName.empty())
				configuration->sections[sectionName] = section;

			sectionName = line.substr(1, index - 1);
			trim(sectionName);

			std::transform(sectionName.begin(), sectionName.end(), sectionName.begin(), tolower);

			ASSERT(!sectionName.empty(), "Section name is empty");
			ASSERT(configuration->sections.find( sectionName ) == configuration->sections.end(),
			       "Section with same name already exists");

			section.clear();
		}
		else
		{
			ASSERT(!sectionName.empty(), "Value is contained within no section");

			size_t index = line.find('=');

			ASSERT(index != std::string::npos, "Expected equality sign after key name");

			std::string key = line.substr(0, index);
			std::string value = line.substr(index + 1);

			trim(key);
			trim(value);

			std::transform(key.begin(), key.end(), key.begin(), tolower);

			ASSERT(!key.empty(), "Key name before equality sign is empty");
			ASSERT(section.find( key ) == section.end(), "Key with same name already exists");

			section[key] = value;
		}
		lineIndex++;
	}

	if (!sectionName.empty())
		configuration->sections[sectionName] = section;

	stream.close();
	return true;

error:
	fprintf(stderr, "[QuickBoot] Failed to parse configuration file: %s\n", filePath.c_str());
	fprintf(stderr, "[QuickBoot] Line %llu: %s\n", lineIndex, errorMsg.c_str());
	stream.close();
	return false;
}
