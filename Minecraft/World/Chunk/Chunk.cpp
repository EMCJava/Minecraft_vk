//
// Created by loys on 5/23/22.
//

#include "Chunk.hpp"

#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>

#include <cmath>

Chunk::~Chunk( )
{
    DeleteChunk( );
}

void
Chunk::RegenerateChunk( )
{
    DeleteChunk( );

    auto worldPercentageHeightMap = GenerateHeightMap( *MinecraftServer::GetInstance( ).GetWorld( ).GetHeightNoise( ) );
    auto worldHeightMap           = std::make_unique<int[]>( SectionSurfaceSize );

    int   heightMapIndex = 0;
    float maxHeight      = 0;
    for ( int i = 0; i < SectionUnitLength; ++i )
        for ( int j = 0; j < SectionUnitLength; ++j )
        {
            worldHeightMap[ heightMapIndex ] = int( worldPercentageHeightMap[ heightMapIndex ] * ChunkMaxHeight );
            maxHeight                        = std::max( maxHeight, worldPercentageHeightMap[ heightMapIndex ] );

            ++heightMapIndex;
        }

    m_sectionCount = std::ceil( MaxSectionInChunk * maxHeight );
    m_Blocks       = new Block[ m_sectionCount * TotalBlocksInSection ];

    auto blocksPtr = m_Blocks;
    for ( int i = 0; i < m_sectionCount * SectionUnitLength; ++i )
    {
        heightMapIndex = 0;
        for ( int k = 0; k < SectionUnitLength; ++k )
            for ( int j = 0; j < SectionUnitLength; ++j )
            {
                blocksPtr[ heightMapIndex ] = worldHeightMap[ heightMapIndex ] > i ? BlockID::Air : BlockID::Stone;

                ++heightMapIndex;
            }

        blocksPtr += SectionSurfaceSize;
    }

    assert(blocksPtr - m_sectionCount * TotalBlocksInSection == m_Blocks);
}

std::unique_ptr<float[]>
Chunk::GenerateHeightMap( const MinecraftNoise& generator )
{
    auto heightMap    = std::make_unique<float[]>( SectionSurfaceSize );
    auto heightMapPtr = heightMap.get( );

    auto chunkX = GetMinecraftX( m_coordinate ) << SectionBinaryOffsetLength;
    auto chunkZ = GetMinecraftZ( m_coordinate ) << SectionBinaryOffsetLength;

    for ( int i = 0; i < SectionUnitLength; ++i )
    {
        for ( int j = 0; j < SectionUnitLength; ++j )
        {
            *( heightMapPtr++ ) = generator.GetNoiseInt( chunkX + j, chunkZ + i );
        }
    }

    return std::move( heightMap );
}
