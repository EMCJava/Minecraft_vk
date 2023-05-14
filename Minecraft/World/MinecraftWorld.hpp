//
// Created by loys on 5/22/22.
//

#ifndef MINECRAFT_VK_MINECRAFTWORLD_HPP
#define MINECRAFT_VK_MINECRAFTWORLD_HPP

#include <Minecraft/Block/Block.hpp>
#include <Minecraft/World/Chunk/ChunkStatus.hpp>
#include <Minecraft/World/Chunk/ChunkPoolType.hpp>
#include <Minecraft/World/Generation/MinecraftNoise.hpp>
#include <Minecraft/util/MinecraftConstants.hpp>
#include <Minecraft/util/MinecraftType.h>
#include <Minecraft/util/Tickable.hpp>

#include <memory>

class ChunkPool;
class MinecraftWorld : public Tickable
{
    std::unique_ptr<float[]>        m_TerrainNoiseOffsetPerLevel;
    std::unique_ptr<MinecraftNoise> m_WorldTerrainNoise;
    std::unique_ptr<MinecraftNoise> m_BedRockNoise;

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
    ~MinecraftWorld( );

    void IntroduceChunkInRange( const ChunkCoordinate& centre, int32_t radius );
    void IntroduceChunk( const ChunkCoordinate& position, ChunkStatus minimumStatus );

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
    static inline ChunkCoordinate BlockToChunkWorldCoordinate( const BlockCoordinate& blockCoordinate ) { return MakeMinecraftChunkCoordinate( ScaleToSecond<SectionUnitLength, 1>( GetMinecraftX( blockCoordinate ) ), ScaleToSecond<SectionUnitLength, 1>( GetMinecraftZ( blockCoordinate ) ) ); }
    static inline BlockCoordinate BlockToChunkRelativeCoordinate( const BlockCoordinate& blockCoordinate ) { return MakeMinecraftCoordinate( GetMinecraftX( blockCoordinate ) & ( SectionUnitLength - 1 ), GetMinecraftY( blockCoordinate ), GetMinecraftZ( blockCoordinate ) & ( SectionUnitLength - 1 ) ); }
    std::shared_ptr<ChunkTy>      GetCompleteChunkCache( const ChunkCoordinate& chunkCoordinate );
    std::shared_ptr<ChunkTy>      GetChunkCacheSafe( const ChunkCoordinate& chunkCoordinate );
    std::shared_ptr<ChunkTy>      GetChunkCacheUnsafe( const ChunkCoordinate& chunkCoordinate );
    Block*                        GetBlock( const BlockCoordinate& blockCoordinate );
    bool                          SetBlock( const BlockCoordinate& blockCoordinate, const Block& block );

    [[nodiscard]] const auto& GetBedRockNoise( ) const { return m_BedRockNoise; }
    [[nodiscard]] const auto& GetTerrainNoise( ) const { return m_WorldTerrainNoise; }
    [[nodiscard]] const auto* GetTerrainNoiseOffset( ) const { return m_TerrainNoiseOffsetPerLevel.get( ); }
    ChunkPool&                GetChunkPool( ) { return *m_ChunkPool; }
};


#endif   // MINECRAFT_VK_MINECRAFTWORLD_HPP
