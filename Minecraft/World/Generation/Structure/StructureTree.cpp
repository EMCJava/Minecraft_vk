//
// Created by loys on 7/26/22.
//

#include "StructureTree.hpp"

#include "Minecraft/World/Chunk/Chunk.hpp"
#include "Minecraft/World/MinecraftWorld.hpp"
#include "Utility/Logger.hpp"

void
StructureTree::Generate( WorldChunk& chunk )
{
    static constexpr auto MIN_HEIGHT = 6;
    static constexpr auto MAX_HEIGHT = 20;
    static_assert( MAX_HEIGHT >= MIN_HEIGHT );

    const WorldChunk& startingChunk      = GetStartingChunk( chunk );
    const auto        startingCoordinate = startingChunk.WorldToChunkRelativeCoordinate( m_StartingPosition );
    const auto        startingIndex      = ToHorizontalIndex( startingCoordinate );

    const auto heightAtPoint = startingChunk.GetHeight( startingIndex, eNoiseHeight ) + 1;
    auto       treeHeight    = std::min( ChunkMaxHeight - heightAtPoint, MAX_HEIGHT - MIN_HEIGHT );
    auto       chunkNoise    = startingChunk.CopyChunkNoise( );

    treeHeight = chunkNoise.NextUint64( ) % treeHeight + MIN_HEIGHT;

    // trunk
    if ( chunk.IsPointInsideHorizontally( m_StartingPosition ) )
    {
        for ( int i = 0; i < treeHeight; ++i )
        {
            chunk.SetBlock( startingCoordinate + MakeMinecraftCoordinate( 0, heightAtPoint + i, 0 ), BlockID::AcaciaLog );
        }
    }

    auto leafOrigin             = m_StartingPosition;
    GetMinecraftY( leafOrigin ) = heightAtPoint + treeHeight;

    for ( int x = -4; x <= 4; ++x )
        for ( int y = -4; y <= 4; ++y )
            for ( int z = -4; z <= 4; ++z )
            {
                const auto leafDensity = std::abs( x ) + std::abs( y ) + std::abs( z );
                if ( chunkNoise.NextUint64( ) % 8 >= leafDensity )
                {
                    SetBlockWorld( chunk, leafOrigin + MakeMinecraftCoordinate( x, y, z ), BlockID::AzaleaLeaves, false );
                }
            }
}

bool
StructureTree::TryGenerate( WorldChunk& chunk, std::vector<std::shared_ptr<Structure>>& structureList )
{
    bool           generated  = false;
    MinecraftNoise chunkNoise = chunk.CopyChunkNoise( );

    int horizontalMapIndex = 0;
    for ( int k = 0; k < SectionUnitLength; ++k )
        for ( int j = 0; j < SectionUnitLength; ++j, ++horizontalMapIndex )
        {
            if ( chunkNoise.NextUint64( ) % ( 16 * 16 * 10 ) == 0 )
            {
                generated          = true;
                auto chunkPosition = chunk.GetWorldCoordinate( );
                auto newStructure  = std::make_shared<StructureTree>( );

                GetMinecraftX( newStructure->m_StartingPosition ) = GetMinecraftX( chunkPosition ) += j;
                GetMinecraftZ( newStructure->m_StartingPosition ) = GetMinecraftZ( chunkPosition ) += k;

                newStructure->AddPiece( { GetMinecraftX( chunkPosition ) - 4.0f, GetMinecraftZ( chunkPosition ) - 4.0f, GetMinecraftX( chunkPosition ) + 4.0f, GetMinecraftZ( chunkPosition ) + 4.0f } );

                structureList.emplace_back( std::move( newStructure ) );
            }
        }

    return generated;
}
