//
// Created by loys on 16/1/2022.
//

#ifndef MINECRAFT_VK_UTILITY_LOGGER_HPP
#define MINECRAFT_VK_UTILITY_LOGGER_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

#include <Utility/String/StringConcatConstexpr.hpp>

template <bool LogColorPrefix = true>
class LoggerBase
{

private:
    template <std::string_view const& Str, std::string_view const&... Strs>
    struct JoinColorSrt {

        static constexpr std::string_view value = LogColorPrefix ? Join<Str, Strs...>::value : Join<Strs...>::value;
    };

    static constexpr std::string_view ErrorStr = "[ ERROR ] ";
    static constexpr std::string_view InfoStr  = "[ INFO ] ";

    static constexpr std::string_view RedStr    = "\033[38;5;9m";
    static constexpr std::string_view YellowStr = "\033[38;5;11m";
    static constexpr std::string_view ResetStr  = "\033[38;5;15m";

    static constexpr auto ErrorPrefix = JoinColorSrt<RedStr, ErrorStr>::value;
    static constexpr auto InfoPrefix  = JoinColorSrt<YellowStr, InfoStr>::value;

public:
    enum class Color {
        wReset,
        eRed,
        eYellow,
        eColorSize
    };

    enum class LogType {
        eError,
        eInfo,
        eLogTypeSize
    };

    LoggerBase( )
    {
        std::ios_base::sync_with_stdio( false );
        std::cin.tie( nullptr );
    }

    LoggerBase( LoggerBase const& )            = delete;
    LoggerBase& operator=( LoggerBase const& ) = delete;
    LoggerBase( LoggerBase&& )                 = delete;
    LoggerBase& operator=( LoggerBase&& )      = delete;

    inline static LoggerBase&
    getInstance( )
    {
        static LoggerBase instance;
        return instance;
    }

    static inline const std::basic_string_view<char>&
    prefix( const LogType& type )
    {
        switch ( type )
        {
        case LogType::eError:
            return ErrorPrefix;
        case LogType::eInfo:
            return InfoPrefix;
        default:
            break;
        }

        return ResetStr;
    }

    static inline const std::basic_string_view<char>&
    prefix( const Color& color )
    {
        switch ( color )
        {
        case Color::wReset:
            return ResetStr;
        case Color::eRed:
            return RedStr;
        case Color::eYellow:
            return YellowStr;
        default:
            break;
        }

        return RedStr;
    }

    template <typename... Tys>
    void
    LogLine( Tys&&... elms )
    {
        Log( std::forward<Tys>( elms )..., '\n' );
    }

    template <typename Ty, typename... Tys>
    void
    Log( Ty&& elm, Tys&&... elms )
    {
        if constexpr ( std::is_same<typename std::decay<Ty>::type, Color>::value || std::is_same<typename std::decay<Ty>::type, LogType>::value )
        {
            LogWithColor( elm, elms... );
        } else
        {
            std::stringstream output;
            output << std::forward<Ty>( elm );
            ( ( output << ' ' << std::forward<Tys>( elms ) ), ... ) << std::flush;

            Log( output.str( ) );
        }
    }

    template <typename Ty, typename... Tys>
    void
    LogWithColor( Ty&& colorElement, Tys&&... elms )
    {
        if constexpr ( LogColorPrefix || !std::is_same<typename std::decay<Ty>::type, Color>::value )
            Log( prefix( colorElement ) );

        Log( std::forward<Tys>( elms )... );

        if constexpr ( LogColorPrefix )
            Log( prefix( Color::wReset ) );
    }

    template <typename Ty>
    inline void
    Log( Ty&& elm )
    {
        std::cout << std::forward<Ty>( elm ) << std::flush;
    }

    template <typename Ty>
    inline LoggerBase&
    operator<<( const Ty& elm )
    {
        Log( elm );
        return *this;
    }
};

using Logger = LoggerBase<false>;

#endif   // MINECRAFT_VK_UTILITY_LOGGER_HPP
