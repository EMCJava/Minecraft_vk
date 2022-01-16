//
// Created by loys on 16/1/2022.
//

#ifndef MINECRAFT_VK_UTILITY_LOGGER_HPP
#define MINECRAFT_VK_UTILITY_LOGGER_HPP

#include <iostream>
#include <string>
#include <sstream>

class Logger
{

private:
	static constexpr auto RED = "\033[38;5;9m";
	static constexpr auto RESET_COLOR = "\033[38;5;15m";

public:

	enum class Color
	{
		wReset, eRed, eColorSize
	};

	inline static Logger &
	getInstance()
	{
		static Logger instance;
		return instance;
	}

	static inline const char *
	prefix(const Color &color)
	{
		switch (color)
		{
			case Color::wReset: return RESET_COLOR;
			case Color::eRed: return RED;
		}
	}

	template<typename ... Ty>
	void
	Log(Color &&color, Ty &&... str)
	{
		Log(prefix(color));
		Log(str...);
		Log(prefix(Color::wReset));
	}

	template<typename ... Ty>
	void
	Log(Ty &&... str)
	{
		std::stringstream output;
		((output << str << ' '), ...) << std::flush;
		Log(output.str());
	}

	template<typename Ty>
	inline void
	Log(Ty &&str)
	{
		std::cout << str << std::flush;
	}

	template<typename Ty>
	inline Logger &
	operator<<(const Ty &str)
	{
		Log(str);
		return *this;
	}
};

#endif //MINECRAFT_VK_UTILITY_LOGGER_HPP
