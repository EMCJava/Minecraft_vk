//
// Created by samsa on 7/2/2022.
//

#ifndef MINECRAFT_VK_UTILITY_TIMER_HPP
#define MINECRAFT_VK_UTILITY_TIMER_HPP

#include "Logger.hpp"

#include <atomic>

template <bool LogOnDeconstruct>
class TTimer
{
    std::chrono::high_resolution_clock::time_point m_StartTime;

public:
    TTimer( ) { Start( ); }
    ~TTimer( )
    {
        if constexpr ( LogOnDeconstruct ) Log( );
    }

    void Log( const std::string& prefix )
    {
        Logger::getInstance( )
            .LogLine(
                Logger::LogType::eInfo,
                "Timer [",
                prefix,
                ']',
                std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::high_resolution_clock::now( ) - m_StartTime ).count( ),
                "mic-s." );
    }

    void Log( )
    {
        Logger::getInstance( )
            .LogLine(
                Logger::LogType::eInfo,
                "Timer",
                std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::high_resolution_clock::now( ) - m_StartTime ).count( ),
                "mic-s." );
    }

    void Start( ) { m_StartTime = std::chrono::high_resolution_clock::now( ); }

    void Step( )
    {
        Log( );
        Start( );
    }
};

using Timer = TTimer<true>;

#endif   // MINECRAFT_VK_UTILITY_TIMER_HPP
