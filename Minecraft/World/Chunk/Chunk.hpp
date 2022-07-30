//
// Created by loys on 5/23/22.
//

#ifndef MINECRAFT_VK_CHUNK_HPP
#define MINECRAFT_VK_CHUNK_HPP

#include <Minecraft/Block/Block.hpp>
#include <Minecraft/World/Generation/MinecraftNoise.hpp>
#include <Minecraft/World/Generation/Structure/Structure.hpp>
#include <Minecraft/util/MinecraftConstants.hpp>
#include <Minecraft/util/MinecraftType.h>

#include "ChunkStatus.hpp"

#include <array>
#include <cmath>
#include <memory>

class Chunk
{

    class MinecraftWorld* m_World;

    ChunkCoordinate m_Coordinate;
    ChunkCoordinate m_WorldCoordinate;

    Block*   m_Blocks { };
    int32_t* m_WorldHeightMap { };

    MinecraftNoise m_ChunkNoise;

    // TODO: to not use shared & weak ptr
    std::vector<std::shared_ptr<Structure>> m_StructureStarts;
    std::vector<std::weak_ptr<Structure>>   m_StructureReferences;

    ChunkStatus m_Status = ChunkStatus::eEmpty;

private:
    inline void DeleteChunk( )
    {
        delete[] m_WorldHeightMap;
        delete[] m_Blocks;
        m_Blocks         = nullptr;
        m_WorldHeightMap = nullptr;

        m_Status = ChunkStatus::eEmpty;
        m_StructureStarts.clear( );
        m_StructureReferences.clear( );
    }

    /*
     *
     * Can only be used when surrounding chunk is loaded
     *
     * */
    void RegenerateChunk( ChunkStatus status );

    void UpgradeChunk( ChunkStatus targetStatus );

    bool CanRunStructureStart( ) const;
    bool CanRunStructureReference( ) const;
    bool CanRunNoise( ) const;
    bool CanRunFeature( ) const;

    bool AttemptRunStructureStart( );
    bool AttemptRunStructureReference( );
    bool AttemptRunNoise( );
    bool AttemptRunFeature( );

    /*
     *
     * Generation
     *
     * */
    [[nodiscard]] const auto& GetStructureStarts( ) const { return m_StructureStarts; }

public:
    Chunk( class MinecraftWorld* world )
        : m_World( world )
    { }
    ~Chunk( );

    Chunk( const Chunk& ) = delete;
    Chunk( Chunk&& )      = delete;

    Chunk operator=( const Chunk& ) = delete;
    Chunk operator=( Chunk&& )      = delete;

    void SetWorld( class MinecraftWorld* world ) { m_World = world; }

    /*
     *
     * Generation
     *
     * */
    void FillTerrain( const MinecraftNoise& generator );
    void FillBedRock( const MinecraftNoise& generator );

    [[nodiscard]] inline auto GetChunkNoise( const MinecraftNoise& terrainGenerator ) const
    {
        auto noiseSeed = terrainGenerator.CopySeed( );
        noiseSeed.first ^= GetMinecraftX( m_WorldCoordinate );
        noiseSeed.second ^= GetMinecraftZ( m_WorldCoordinate );

        return MinecraftNoise { noiseSeed };
    }

    /*
     *
     * Stats
     *
     * */
    CoordinateType GetHeight( uint32_t index );

    [[nodiscard]] bool IsChunkStatusAtLeast( ChunkStatus status ) const { return m_Status >= status; }

    /*
     *
     * Access tools
     *
     * */
    [[nodiscard]] const Block* CheckBlock( const BlockCoordinate& blockCoordinate ) const;
    [[nodiscard]] Block*       GetBlock( const BlockCoordinate& blockCoordinate );

    bool SetBlock( const BlockCoordinate& blockCoordinate, const Block& block );
    bool SetBlock( const uint32_t& blockIndex, const Block& block );

    void SetCoordinate( const ChunkCoordinate& coordinate );

    inline auto GetChunkNoise( ) { return m_ChunkNoise; }

    inline const ChunkCoordinate& GetCoordinate( ) { return m_Coordinate; }

    [[nodiscard]] inline CoordinateType ManhattanDistance( const Chunk& other ) const
    {
        return ManhattanDistance( other.m_Coordinate );
    }

    [[nodiscard]] inline CoordinateType ManhattanDistance( const ChunkCoordinate& other ) const
    {
        return std::abs( std::get<0>( m_Coordinate ) - std::get<0>( other ) )
            + std::abs( std::get<1>( m_Coordinate ) - std::get<1>( other ) )
            + std::abs( std::get<2>( m_Coordinate ) - std::get<2>( other ) );
    }

    friend class ChunkCache;
};


#endif   // MINECRAFT_VK_CHUNK_HPP
