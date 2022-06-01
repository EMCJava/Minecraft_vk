//
// Created by loys on 5/24/22.
//

#ifndef MINECRAFT_VK_CHUNKCACHE_HPP
#define MINECRAFT_VK_CHUNKCACHE_HPP

#include "Chunk.hpp"

#include <Minecraft/util/MinecraftType.h>

#include <Utility/Thread/ThreadPool.hpp>

#include <unordered_map>

class ChunkCache
{
public:
    Chunk chunk;
    bool  initialized  = false;
    bool  initializing = false;

    void ResetLoad( );
};


#endif   // MINECRAFT_VK_CHUNKCACHE_HPP
