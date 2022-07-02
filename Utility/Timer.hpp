//
// Created by samsa on 7/2/2022.
//

#ifndef MINECRAFT_VK_UTILITY_TIMER_HPP
#define MINECRAFT_VK_UTILITY_TIMER_HPP

#include "Logger.hpp"

class Timer
{
    std::chrono::high_resolution_clock::time_point startTime;

public:
    Timer( )
        : startTime( std::chrono::high_resolution_clock::now( ) ) { };
    ~Timer( )
    {
        Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Timer:", std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::high_resolution_clock::now( ) - startTime ).count( ), "ms." );
    }
};

#endif   // MINECRAFT_VK_UTILITY_TIMER_HPP
