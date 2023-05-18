//
// Created by loys on 7/26/22.
//

#include "StructureTree.hpp"

#include "Minecraft/World/Chunk/Chunk.hpp"
#include "Minecraft/World/Chunk/WorldChunk.hpp"
#include "Minecraft/World/MinecraftWorld.hpp"
#include "Utility/Logger.hpp"

void
StructureTree::Generate( WorldChunk& chunk )
{
    static constexpr auto MIN_HEIGHT = 5;
    static constexpr auto MAX_HEIGHT = 7;
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
            SetBlock( chunk, BlockID::AcaciaLog, startingCoordinate + MakeMinecraftCoordinate( 0, heightAtPoint + i, 0 ), false );
        }
    }

    auto leafOrigin             = m_StartingPosition;
    GetMinecraftY( leafOrigin ) = heightAtPoint + treeHeight;

    SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin, false );

    for ( int i = 0; i < 2; ++i )
    {
        SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( 1, i, 0 ), false );
        SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( -1, i, 0 ), false );
        SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( 0, i, 1 ), false );
        SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( 0, i, -1 ), false );
    }

    if ( chunkNoise.NextUint64( ) % 3 == 2 ) SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( 1, 1, 1 ), false );
    if ( chunkNoise.NextUint64( ) % 3 == 2 ) SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( -1, 1, 1 ), false );
    if ( chunkNoise.NextUint64( ) % 3 == 2 ) SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( 1, 1, -1 ), false );
    if ( chunkNoise.NextUint64( ) % 3 == 2 ) SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( -1, 1, -1 ), false );

    for ( int i = 2; i < 4; ++i )
        for ( int x = -2; x <= 2; ++x )
            for ( int y = -2; y <= 2; ++y )
            {
                if ( std::abs( x ) + std::abs( y ) <= 3 )
                {
                    SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( x, i, y ), false );
                }
            }

    if ( chunkNoise.NextUint64( ) % 2 == 1 )
    {
        SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( 2, 3, 2 ), false );
        if ( chunkNoise.NextUint64( ) % 3 == 2 ) SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( 2, 2, 2 ), false );
    }
    if ( chunkNoise.NextUint64( ) % 2 == 1 )
    {
        SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( -2, 3, 2 ), false );
        if ( chunkNoise.NextUint64( ) % 3 == 2 ) SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( -2, 2, 2 ), false );
    }
    if ( chunkNoise.NextUint64( ) % 2 == 1 )
    {
        SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( 2, 3, -2 ), false );
        if ( chunkNoise.NextUint64( ) % 3 == 2 ) SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( 2, 2, -2 ), false );
    }
    if ( chunkNoise.NextUint64( ) % 2 == 1 )
    {
        SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( -2, 3, -2 ), false );
        if ( chunkNoise.NextUint64( ) % 3 == 2 ) SetBlockWorld( chunk, chunkNoise.NextUint64( ) % 8 == 0 ? BlockID::FloweringAzaleaLeaves : BlockID::AzaleaLeaves, leafOrigin - MakeMinecraftCoordinate( -2, 2, -2 ), false );
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
