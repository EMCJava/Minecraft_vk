//
// Created by loys on 5/23/22.
//

#ifndef MINECRAFT_VK_CHUNK_HPP
#define MINECRAFT_VK_CHUNK_HPP

#include <Minecraft/Block/Block.hpp>
#include <Minecraft/util/MinecraftConstants.hpp>
#include <Minecraft/util/MinecraftType.h>

#include <cmath>

class Chunk
{

    ChunkCoordinate m_coordinate;

    Block* m_Blocks { };

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
