//
// Created by loys on 5/23/22.
//

#ifndef MINECRAFT_VK_CHUNK_HPP
#define MINECRAFT_VK_CHUNK_HPP

#include <Minecraft/Block/Block.hpp>
#include <Minecraft/util/MinecraftConstants.hpp>
#include <Minecraft/util/MinecraftType.h>
#include <Minecraft/World/Generation/MinecraftNoise.hpp>

#include <cmath>

class Chunk
{

    ChunkCoordinate m_coordinate;

    int    m_sectionCount = 0;
    Block* m_Blocks { };

private:
    inline void DeleteChunk( )
    {
        delete[] m_Blocks;
        m_Blocks = nullptr;
    }

    std::unique_ptr<float[]> GenerateHeightMap( const MinecraftNoise& );

    void RegenerateChunk( );

public:
    Chunk( ) = default;
    ~Chunk( );

    Chunk( const Chunk& ) = delete;
    Chunk( Chunk&& )      = delete;

    Chunk operator=( const Chunk& ) = delete;
    Chunk operator=( Chunk&& )      = delete;

    inline const ChunkCoordinate& SetCoordinate( const ChunkCoordinate& coordinate ) { return m_coordinate = coordinate; }
    inline const ChunkCoordinate& GetCoordinate( ) { return m_coordinate; }

    [[nodiscard]] inline CoordinateType ManhattanDistance( const Chunk& other ) const
    {
        return ManhattanDistance( other.m_coordinate );
    }

    [[nodiscard]] inline CoordinateType ManhattanDistance( const ChunkCoordinate& other ) const
    {
        return std::abs( std::get<0>( m_coordinate ) - std::get<0>( other ) )
            + std::abs( std::get<1>( m_coordinate ) - std::get<1>( other ) )
            + std::abs( std::get<2>( m_coordinate ) - std::get<2>( other ) );
    }

    friend class ChunkCache;
};


#endif   // MINECRAFT_VK_CHUNK_HPP
