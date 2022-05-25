//
// Created by loys on 5/24/22.
//

#ifndef MINECRAFT_VK_CHUNKCACHE_HPP
#define MINECRAFT_VK_CHUNKCACHE_HPP

#include "Chunk.hpp"

#include <Minecraft/util/MinecraftType.h>

#include <Utility/Logger.hpp>
#include <Utility/Thread/ThreadPool.hpp>

#include <unordered_map>

class ChunkCache
{
public:
    Chunk chunk;
    bool  initialized  = false;
    bool  initializing = false;

    void ResetLoad( )
    {
//        Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Loading [", std::get<0>( chunk.GetCoordinate( ) ), "][",
//                                        std::get<1>( chunk.GetCoordinate( ) ), "][", std::get<2>( chunk.GetCoordinate( ) ), "]" );
    }
};


#endif   // MINECRAFT_VK_CHUNKCACHE_HPP
