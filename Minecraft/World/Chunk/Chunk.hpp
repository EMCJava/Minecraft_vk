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

class Chunk : public AABB
{
protected:
    class MinecraftWorld* m_World;

    ChunkCoordinate m_Coordinate;
    ChunkCoordinate m_WorldCoordinate;

    std::unique_ptr<Block[]>   m_Blocks { };
    std::unique_ptr<int32_t[]> m_HeightMap { };

public:
    explicit Chunk( class MinecraftWorld* world )
        : m_World( world )
    { }

    Chunk( const Chunk& ) = delete;
    Chunk( Chunk&& )      = delete;

    Chunk operator=( const Chunk& ) = delete;
    Chunk operator=( Chunk&& )      = delete;

    void SetWorld( class MinecraftWorld* world ) { m_World = world; }

    /*
     *
     * Stats
     *
     * */
    CoordinateType GetHeight( uint32_t index );

    /*
     *
     * Access tools
     *
     * */
    [[nodiscard]] const Block* CheckBlock( const BlockCoordinate& blockCoordinate ) const;
    [[nodiscard]] Block*       GetBlock( const BlockCoordinate& blockCoordinate );

    bool SetBlockAtWorldCoordinate( const BlockCoordinate& blockCoordinate, const Block& block );
    bool SetBlock( const BlockCoordinate& blockCoordinate, const Block& block );
    bool SetBlock( const uint32_t& blockIndex, const Block& block );

    void SetCoordinate( const ChunkCoordinate& coordinate );

    inline auto&                  GetWorld( ) const { return m_World; }
    inline BlockCoordinate        WorldToChunkRelativeCoordinate( const BlockCoordinate& blockCoordinate ) const { return blockCoordinate - m_WorldCoordinate; }
    inline const ChunkCoordinate& GetChunkCoordinate( ) const { return m_Coordinate; }
    inline const ChunkCoordinate& GetWorldCoordinate( ) const { return m_WorldCoordinate; }
    inline bool                   IsPointInside( const BlockCoordinate& blockCoordinate ) const { return IsPointInsideVertically( blockCoordinate ) && IsPointInsideHorizontally( blockCoordinate ); }
    inline bool                   IsPointInsideVertically( const BlockCoordinate& blockCoordinate ) const { return GetMinecraftY( blockCoordinate ) < ChunkMaxHeight && GetMinecraftY( blockCoordinate ) >= 0; }
    inline bool                   IsPointInsideHorizontally( const BlockCoordinate& blockCoordinate ) const
    {
        return GetMinecraftX( blockCoordinate ) >= GetMinecraftX( m_WorldCoordinate ) && GetMinecraftX( blockCoordinate ) < GetMinecraftX( m_WorldCoordinate ) + SectionUnitLength && GetMinecraftZ( blockCoordinate ) >= GetMinecraftZ( m_WorldCoordinate ) && GetMinecraftZ( blockCoordinate ) < GetMinecraftZ( m_WorldCoordinate ) + SectionUnitLength;
    }

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
};


#endif   // MINECRAFT_VK_CHUNK_HPP
