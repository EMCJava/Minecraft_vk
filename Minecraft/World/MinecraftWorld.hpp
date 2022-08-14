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

    void        IntroduceChunkInRange( const ChunkCoordinate& centre, int32_t radius );
    ChunkCache* IntroduceChunk( const ChunkCoordinate& position, ChunkStatus minimumStatus );

    /*
     *
     * Chunk generation
     *
     * */
    void StopChunkGeneration( );
    void StartChunkGeneration( );
    void CleanChunk( );

    void SetTerrainNoiseOffset( std::unique_ptr<float[]>&& data ) { m_TerrainNoiseOffsetPerLevel = std::move( data ); }

    auto& GetModifiableTerrainNoise( ) { return *m_WorldTerrainNoise; }

    /*
     *
     * Tickable
     *
     * */
    void Tick( float deltaTime );

    /*
     *
     * Access tools
     *
     * */
    ChunkCache* GetCompleteChunkCache( const ChunkCoordinate& chunkCoordinate );
    ChunkCache* GetChunkCache( const ChunkCoordinate& chunkCoordinate );
    Block*      GetBlock( const BlockCoordinate& blockCoordinate );
    bool        SetBlock( const BlockCoordinate& blockCoordinate, const Block& block );

    [[nodiscard]] const auto& GetBedRockNoise( ) const { return m_BedRockNoise; }
    [[nodiscard]] const auto& GetTerrainNoise( ) const { return m_WorldTerrainNoise; }
    [[nodiscard]] const auto* GetTerrainNoiseOffset( ) const { return m_TerrainNoiseOffsetPerLevel.get( ); }
    ChunkPool&                GetChunkPool( ) { return *m_ChunkPool; }
};


#endif   // MINECRAFT_VK_MINECRAFTWORLD_HPP
