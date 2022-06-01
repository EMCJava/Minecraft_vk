//
// Created by loys on 5/22/22.
//

#ifndef MINECRAFT_VK_MINECRAFTWORLD_HPP
#define MINECRAFT_VK_MINECRAFTWORLD_HPP

#include <Minecraft/World/Chunk/ChunkPool.hpp>
#include <Minecraft/util/Tickable.hpp>

#include <memory>

class MinecraftWorld : public Tickable
{
    std::unique_ptr<ChunkPool> m_ChunkPool;

    /*
     *
     * Config
     *
     * */
    CoordinateType m_ChunkLoadingRange { };

    float m_TimeSinceChunkLoad { };

public:
    MinecraftWorld( );

    void IntroduceChunkInRange( ChunkCoordinate centre, int32_t radius );

    void Tick( float deltaTime );
};


#endif   // MINECRAFT_VK_MINECRAFTWORLD_HPP
