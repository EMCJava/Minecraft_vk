//
// Created by samsa on 7/2/2022.
//

#ifndef MINECRAFT_VK_UTILITY_TIMER_HPP
#define MINECRAFT_VK_UTILITY_TIMER_HPP

#include "Logger.hpp"

namespace
{
std::atomic<uint32_t> TimerInstanceCounter( 0 );
};

class Timer
{
    std::chrono::high_resolution_clock::time_point startTime;

public:
    Timer( )
        : startTime( std::chrono::high_resolution_clock::now( ) )
    {
        ++TimerInstanceCounter;
    };

    ~Timer( )
    {
        Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Timer[", TimerInstanceCounter, "]:", std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::high_resolution_clock::now( ) - startTime ).count( ), "mic-s." );
        --TimerInstanceCounter;
    }
};

#endif   // MINECRAFT_VK_UTILITY_TIMER_HPP
