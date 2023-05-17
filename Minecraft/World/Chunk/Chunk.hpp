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

#include <Utility/Profiler/Profilable.hpp>

#include "ChunkStatus.hpp"

#include <array>
#include <cmath>
#include <memory>

class Chunk : public AABB
    , public Profilable
{
protected:
    class MinecraftWorld* m_World;

    ChunkCoordinate m_Coordinate;
    BlockCoordinate m_WorldCoordinate;

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

    [[nodiscard]] inline Block& At( const uint32_t index ) const
    {
        assert( m_Blocks != nullptr && index < ChunkVolume );
        return m_Blocks[ index ];
    }

    static inline auto indexToHeight( const auto& index ) { return index >> SectionSurfaceSizeBinaryOffset; }
    static inline int  GetBlockIndex( const BlockCoordinate& blockCoordinate )
    {
        return ScaleToSecond<1, SectionSurfaceSize>( GetMinecraftY( blockCoordinate ) ) + ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( blockCoordinate ) ) + GetMinecraftX( blockCoordinate );
    }

    bool SetBlockAtWorldCoordinate( const BlockCoordinate& blockCoordinate, const Block& block, bool replace = true );
    bool SetBlock( const BlockCoordinate& blockCoordinate, const Block& block, bool replace = true );
    bool SetBlock( const uint32_t& blockIndex, const Block& block, bool replace = true );

    void SetCoordinate( const ChunkCoordinate& coordinate );

    inline auto&                  GetWorld( ) const { return m_World; }
    inline BlockCoordinate        WorldToChunkRelativeCoordinate( const BlockCoordinate& blockCoordinate ) const { return blockCoordinate - m_WorldCoordinate; }
    inline const ChunkCoordinate& GetChunkCoordinate( ) const { return m_Coordinate; }
    inline const BlockCoordinate& GetWorldCoordinate( ) const { return m_WorldCoordinate; }
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
        return ::ManhattanDistance( m_Coordinate, other );
    }

    [[nodiscard]] inline CoordinateType MaxAxisDistance( const Chunk& other ) const
    {
        return MaxAxisDistance( other.m_Coordinate );
    }

    [[nodiscard]] inline CoordinateType MaxAxisDistance( const ChunkCoordinate& other ) const
    {
        return ::MaxAxisDistance( m_Coordinate, other );
    }

    size_t GetObjectSize( ) const;
};


#endif   // MINECRAFT_VK_CHUNK_HPP
