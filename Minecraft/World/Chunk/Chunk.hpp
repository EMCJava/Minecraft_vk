//
// Created by loys on 5/23/22.
//

#ifndef MINECRAFT_VK_CHUNK_HPP
#define MINECRAFT_VK_CHUNK_HPP

#include <Minecraft/Block/Block.hpp>
#include <Minecraft/World/Generation/MinecraftNoise.hpp>
#include <Minecraft/util/MinecraftConstants.hpp>
#include <Minecraft/util/MinecraftType.h>

#include <array>
#include <cmath>

class Chunk
{

    ChunkCoordinate m_Coordinate;
    ChunkCoordinate m_WorldCoordinate;

    Block*    m_Blocks { };
    int32_t* m_WorldHeightMap { };
    uint8_t*  m_BlockFaces { };

    int m_VisibleFacesCount = 0;

    std::array<Chunk*, DirHorizontalSize> m_NearChunks { };
    uint8_t                               m_EmptySlot = ( 1 << DirHorizontalSize ) - 1;

private:
    inline void DeleteChunk( )
    {
        delete[] m_WorldHeightMap;
        delete[] m_BlockFaces;
        delete[] m_Blocks;
        m_Blocks         = nullptr;
        m_BlockFaces     = nullptr;
        m_WorldHeightMap = nullptr;
    }

    void RegenerateVisibleFaces( );
    void RegenerateChunk( );

public:
    Chunk( ) = default;
    ~Chunk( );

    Chunk( const Chunk& ) = delete;
    Chunk( Chunk&& )      = delete;

    Chunk operator=( const Chunk& ) = delete;
    Chunk operator=( Chunk&& )      = delete;

    void FillTerrain( const MinecraftNoise& generator );
    void FillBedRock( const MinecraftNoise& generator );

    // return true if target chunk become complete
    bool SyncChunkFromDirection( Chunk* other, Direction fromDir, bool changes = false );

    inline void SetCoordinate( const ChunkCoordinate& coordinate )
    {
        m_WorldCoordinate = m_Coordinate = coordinate;
        GetMinecraftX( m_WorldCoordinate ) <<= SectionUnitLengthBinaryOffset;
        GetMinecraftZ( m_WorldCoordinate ) <<= SectionUnitLengthBinaryOffset;
    }

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
