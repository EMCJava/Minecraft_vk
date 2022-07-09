//
// Created by loys on 5/22/22.
//

#ifndef MINECRAFT_VK_MINECRAFTWORLD_HPP
#define MINECRAFT_VK_MINECRAFTWORLD_HPP

#include <Minecraft/World/Chunk/ChunkPool.hpp>
#include <Minecraft/World/Generation/MinecraftNoise.hpp>
#include <Minecraft/util/Tickable.hpp>

#include <memory>

class MinecraftWorld : public Tickable
{
    std::unique_ptr<ChunkPool> m_ChunkPool;

    std::unique_ptr<float[]>        m_TerrainNoiseOffsetPerLevel;
    std::unique_ptr<MinecraftNoise> m_WorldTerrainNoise;
    std::unique_ptr<MinecraftNoise> m_BedRockNoise;

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

    void StopChunkGeneration( );
    void StartChunkGeneration( );
    void CleanChunk( );

    void Tick( float deltaTime );

    void SetTerrainNoiseOffset( std::unique_ptr<float[]>&& data ) { m_TerrainNoiseOffsetPerLevel = std::move( data ); }

    const auto& GetBedRockNoise( ) { return m_BedRockNoise; }
    const auto& GetTerrainNoise( ) { return m_WorldTerrainNoise; }
    const auto* GetTerrainNoiseOffset( ) { return m_TerrainNoiseOffsetPerLevel.get( ); }
    ChunkPool&  GetChunkPool( ) { return *m_ChunkPool; }
};


#endif   // MINECRAFT_VK_MINECRAFTWORLD_HPP
