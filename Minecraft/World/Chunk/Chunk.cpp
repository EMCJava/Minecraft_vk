//
// Created by loys on 5/23/22.
//

#include "Chunk.hpp"

#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>

#include <cmath>   // namespace

Chunk::~Chunk( )
{
    DeleteChunk( );
}

void
Chunk::RegenerateChunk( )
{
    DeleteChunk( );

    m_Blocks = new Block[ ChunkVolume ];

    FillTerrain( *MinecraftServer::GetInstance( ).GetWorld( ).GetTerrainNoise( ) );
    FillBedRock( *MinecraftServer::GetInstance( ).GetWorld( ).GetBedRockNoise( ) );
    FillTree( *MinecraftServer::GetInstance( ).GetWorld( ).GetTerrainNoise( ) );

    // Grass
    int  horizontalMapIndex = 0;
    auto blocksPtr          = m_Blocks;
    for ( int i = 0; i < ChunkMaxHeight; ++i )
    {
        horizontalMapIndex = 0;
        for ( int k = 0; k < SectionUnitLength; ++k )
            for ( int j = 0; j < SectionUnitLength; ++j )
            {
                if ( i > m_WorldHeightMap[ horizontalMapIndex ] - 3 )
                {
                    if ( i == m_WorldHeightMap[ horizontalMapIndex ] )
                    {
                        blocksPtr[ horizontalMapIndex ] = BlockID::Grass;
                    } else if ( i <= m_WorldHeightMap[ horizontalMapIndex ] )
                    {
                        blocksPtr[ horizontalMapIndex ] = BlockID::Dart;
                    }
                }

                ++horizontalMapIndex;
            }

        blocksPtr += SectionSurfaceSize;
    }

//        if ( ManhattanDistance( { 0, 0, 0 } ) != 0 )
//            for ( int i = 0; i < ChunkVolume; ++i )
//                m_Blocks[ i ] = BlockID::Air;
}

void
Chunk::FillTerrain( const MinecraftNoise& generator )
{
    auto xCoordinate = GetMinecraftX( m_WorldCoordinate );
    auto zCoordinate = GetMinecraftZ( m_WorldCoordinate );

    const auto& noiseOffset = MinecraftServer::GetInstance( ).GetWorld( ).GetTerrainNoiseOffset( );

    delete[] m_WorldHeightMap;
    m_WorldHeightMap = new int32_t[ SectionSurfaceSize ];
    for ( int i = 0; i < SectionSurfaceSize; ++i )
        m_WorldHeightMap[ i ] = -1;

    auto     blocksPtr          = m_Blocks;
    uint32_t horizontalMapIndex = 0;
    for ( int i = 0; i < ChunkMaxHeight; ++i )
    {
        horizontalMapIndex = 0;
        for ( int k = 0; k < SectionUnitLength; ++k )
            for ( int j = 0; j < SectionUnitLength; ++j )
            {
                auto noiseValue = generator.GetNoiseInt( xCoordinate + j, i, zCoordinate + k );
                noiseValue += noiseOffset[ i ];

                blocksPtr[ horizontalMapIndex ] = noiseValue > 0 ? BlockID::Air : BlockID::Stone;
                if ( !blocksPtr[ horizontalMapIndex ].Transparent( ) ) m_WorldHeightMap[ horizontalMapIndex ] = i;

                ++horizontalMapIndex;
            }

        blocksPtr += SectionSurfaceSize;
    }
}

void
Chunk::FillBedRock( const MinecraftNoise& generator )
{
    auto xCoordinate = GetMinecraftX( m_WorldCoordinate );
    auto zCoordinate = GetMinecraftZ( m_WorldCoordinate );

    auto blackRockHeightMap = std::make_unique<uint32_t[]>( SectionSurfaceSize );

    int horizontalMapIndex = 0;
    for ( int k = 0; k < SectionUnitLength; ++k )
        for ( int j = 0; j < SectionUnitLength; ++j )
        {
            const auto& noiseValue                   = generator.GetNoiseInt( xCoordinate + j, zCoordinate + k ) + 1;
            blackRockHeightMap[ horizontalMapIndex ] = noiseValue * 2 + 1;

            for ( int i = 0; i < blackRockHeightMap[ horizontalMapIndex ]; ++i )
                m_Blocks[ horizontalMapIndex + SectionSurfaceSize * i ] = BlockID ::BedRock;

            ++horizontalMapIndex;
        }
}

const Block*
Chunk::CheckBlock( const BlockCoordinate& blockCoordinate ) const
{
    if ( GetMinecraftY( blockCoordinate ) >= ChunkMaxHeight || GetMinecraftY( blockCoordinate ) < 0 ) return nullptr;
    const auto& blockIndex = ScaleToSecond<1, SectionSurfaceSize>( GetMinecraftY( blockCoordinate ) ) + ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( blockCoordinate ) ) + GetMinecraftX( blockCoordinate );
    assert( blockIndex >= 0 && blockIndex < ChunkVolume );
    return &m_Blocks[ blockIndex ];
}

Block*
Chunk::GetBlock( const BlockCoordinate& blockCoordinate )
{
    if ( GetMinecraftY( blockCoordinate ) >= ChunkMaxHeight || GetMinecraftY( blockCoordinate ) < 0 ) return nullptr;
    const auto& blockIndex = ScaleToSecond<1, SectionSurfaceSize>( GetMinecraftY( blockCoordinate ) ) + ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( blockCoordinate ) ) + GetMinecraftX( blockCoordinate );
    assert( blockIndex >= 0 && blockIndex < ChunkVolume );
    return &m_Blocks[ blockIndex ];
}

void
Chunk::FillTree( const MinecraftNoise& generator )
{
    MinecraftNoise chunkNoise = GetChunkNoise( generator );

    for ( int horizontalMapIndex = 0; horizontalMapIndex < SectionUnitLength; ++horizontalMapIndex )
    {
        if ( chunkNoise.NextUint64( ) % 80 == 0 )
        {

            auto heightAtPoint = m_WorldHeightMap[ horizontalMapIndex ] + 1;
            auto treeHeight    = std::min( ChunkMaxHeight - heightAtPoint, 7 );
            for ( int i = 0; i < treeHeight; ++i )
            {
                m_Blocks[ horizontalMapIndex + ScaleToSecond<1, SectionSurfaceSize>( heightAtPoint + i ) ] = BlockID::AcaciaLog;
            }
        }
    }
}

bool
Chunk::SetBlock( const BlockCoordinate& blockCoordinate, const Block& block )
{
    const auto& blockIndex = ScaleToSecond<1, SectionSurfaceSize>( GetMinecraftY( blockCoordinate ) ) + ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( blockCoordinate ) ) + GetMinecraftX( blockCoordinate );
    return SetBlock( blockIndex, block );
}

bool
Chunk::SetBlock( const uint32_t& blockIndex, const Block& block )
{
    assert( blockIndex >= 0 && blockIndex < ChunkVolume );

    Logger::getInstance( ).LogLine( Logger::LogType::eVerbose, "Setting block at chunk:", m_Coordinate, "index:", blockIndex, "from:", toString( (BlockID) m_Blocks[ blockIndex ] ), "to:", toString( (BlockID) block ) );

    if ( m_Blocks[ blockIndex ] == block ) return false;

    m_Blocks[ blockIndex ] = block;
    return true;
}
