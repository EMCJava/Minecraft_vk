//
// Created by loys on 16/1/2022.
//

#ifndef MINECRAFT_VK_UTILITY_LOGGER_HPP
#define MINECRAFT_VK_UTILITY_LOGGER_HPP

#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <type_traits>

#include <Utility/String/StringConcatConstexpr.hpp>


template <bool UseMutex = true>
class LogFunction
{
    template <typename Ty>
    inline void
    LogFunc( Ty&& elm );
};

template <>
class LogFunction<true>
{
public:
    std::mutex output_mutex;

    template <typename Ty>
    inline void
    LogFunc( Ty&& elm )
    {
        std::lock_guard<std::mutex> lock( output_mutex );
        std::cout << std::forward<Ty>( elm ) << std::flush;
    }
};

template <>
class LogFunction<false>
{
public:
    template <typename Ty>
    inline void
    LogFunc( Ty&& elm )
    {
        std::cout << std::forward<Ty>( elm ) << std::flush;
    }
};


template <bool UseMutex = true, bool LogColorPrefix = true, bool IncludeTime = true>
class LoggerBase : public LogFunction<UseMutex>
{

private:
    template <std::string_view const& Str, std::string_view const&... Strs>
    struct JoinColorSrt {

        static constexpr std::string_view value = LogColorPrefix ? Join<Str, Strs...>::value : Join<Strs...>::value;
    };

    static constexpr std::string_view ErrorStr   = "[ ERROR ] ";
    static constexpr std::string_view WarnStr    = "[ WARN ] ";
    static constexpr std::string_view InfoStr    = "[ INFO ] ";
    static constexpr std::string_view VerboseStr = "[ VERB ] ";
    static constexpr std::string_view SystemStr = "[ SYS ] ";

    static constexpr std::string_view RedStr    = "\033[38;5;9m";
    static constexpr std::string_view YellowStr = "\033[38;5;11m";
    static constexpr std::string_view GrayStr   = "\033[38;5;7m";
    static constexpr std::string_view GreenStr  = "\033[38;5;10m";
    static constexpr std::string_view ResetStr  = "\033[38;5;15m";

    static constexpr auto ErrorPrefix   = JoinColorSrt<RedStr, ErrorStr>::value;
    static constexpr auto WarnPrefix    = JoinColorSrt<YellowStr, WarnStr>::value;
    static constexpr auto InfoPrefix    = JoinColorSrt<YellowStr, InfoStr>::value;
    static constexpr auto VerbosePrefix = JoinColorSrt<GrayStr, VerboseStr>::value;
    static constexpr auto SystemPrefix = JoinColorSrt<GreenStr, SystemStr>::value;

public:
    enum class Color {
        wReset,
        eRed,
        eYellow,
        eGray,
        eGreen,
        eColorSize
    };

    enum class LogType {
        eError,
        eSystem,
        eWarn,
        eInfo,
        eVerbose,
        eLogTypeSize
    };

    LoggerBase( )
    {
        std::ios_base::sync_with_stdio( false );
        std::cin.tie( nullptr );

        LogLine( LogType::eInfo, "Disabled io sync" );
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
        case LogType::eSystem:
            return SystemPrefix;
        case LogType::eWarn:
            return WarnPrefix;
        case LogType::eInfo:
            return InfoPrefix;
        case LogType::eVerbose:
            return VerbosePrefix;
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
        case Color::eGray:
            return GrayStr;
        case Color::eGreen:
            return GreenStr;
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
            LogWithPrefix( elm, elms... );
        } else
        {
            std::stringstream output;

            if constexpr ( IncludeTime )
            {
                std::time_t t = std::time( nullptr );
                output << "[ " << std::put_time( std::localtime( &t ), "%F %T" ) << " ] ";
            }

            output << std::forward<Ty>( elm );
            ( ( output << ' ' << std::forward<Tys>( elms ) ), ... );
            output << std::flush;

            this->LogFunc( output.str( ) );
        }
    }

    template <typename Ty, typename... Tys>
    void
    LogWithPrefix( Ty&& colorElement, Tys&&... elms )
    {
        std::stringstream output;

        if constexpr ( IncludeTime )
        {
            std::time_t t = std::time( nullptr );
            output << "[ " << std::put_time( std::localtime( &t ), "%F %T" ) << " ] ";
        }

        if constexpr ( LogColorPrefix || !std::is_same<typename std::decay<Ty>::type, Color>::value )
            output << prefix( colorElement );

        ( ( output << ' ' << std::forward<Tys>( elms ) ), ... );

        if constexpr ( LogColorPrefix )
            output << prefix( Color::wReset );

        output << std::flush;

        this->LogFunc( output.str( ) );
    }

    template <typename Ty>
    inline LoggerBase&
    operator<<( const Ty& elm )
    {
        Log( elm );
        return *this;
    }
};

using Logger = LoggerBase<false, false>;

#define LOG_EXPRESSION( x ) Logger::getInstance( ).LogLine( #x " = [", ( x ), ']');

#define LOGL_SYS(...) Logger::getInstance().LogLine( Logger::LogType::eSystem, __VA_ARGS__ );
#define LOGL_WARN(...) Logger::getInstance().LogLine( Logger::LogType::eWarn, __VA_ARGS__ );
#define LOGL_INFO(...) Logger::getInstance().LogLine( Logger::LogType::eInfo, __VA_ARGS__ );

#endif   // MINECRAFT_VK_UTILITY_LOGGER_HPP
