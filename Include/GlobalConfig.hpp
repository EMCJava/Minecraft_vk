//
// Created by loys on 16/1/2022.
//

#ifndef MINECRAFT_VK_INCLUDE_GLOBALCONFIG_HPP
#define MINECRAFT_VK_INCLUDE_GLOBALCONFIG_HPP

#include <Include/nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>

namespace
{

struct GlobalConfigStaticList
{
	explicit GlobalConfigStaticList(const std::string &var_name, const std::string &default_value)
	{
		getMap().emplace(var_name, default_value);
	}

	static std::unordered_map<std::string, std::string> &
	getMap()
	{
		static std::unordered_map<std::string, std::string> smap;
		return smap;
	}
};

#define ADD_GLOBAL_CONFIG_VARIABLE(var_name, default_value) \
GlobalConfigStaticList __##var_name##__ = GlobalConfigStaticList(#var_name, #default_value);

ADD_GLOBAL_CONFIG_VARIABLE(window_resizable, false)
ADD_GLOBAL_CONFIG_VARIABLE(screen_width, 800)
ADD_GLOBAL_CONFIG_VARIABLE(screen_height, 600)
}

struct GlobalConfig
{
	static inline nlohmann::json&
	getConfigData()
	{
		static GlobalConfig gc;
		(void)gc;

		static nlohmann::json j;
		return j;
	}

	static void
	LoadFromFile(const std::string &path)
	{

		std::ifstream file(path);

		if (file)
			file >> getConfigData();

		bool hasData = !getConfigData().is_null();
		if(hasData){
			std::cout << "GlobalConfig:" << std::endl;
			std::cout << getConfigData().dump(4) << std::endl;
		}

		// initialize config instance
		(void)getConfigData();

		for (const auto &config: GlobalConfigStaticList::getMap())
		{
			if (hasData)
			{
				auto data = getConfigData()[config.first];
				if (!data.is_null())
				{
					continue;
				}
			}

			std::cerr << "Can't find " << config.first << " in " << path << " using " << config.second
			          << " as default value" << std::endl;
			getConfigData().emplace(config.first, config.second);
		}

		file.close();
	}
};

#endif //MINECRAFT_VK_INCLUDE_GLOBALCONFIG_HPP
