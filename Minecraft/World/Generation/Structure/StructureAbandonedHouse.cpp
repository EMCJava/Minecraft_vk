//
// Created by samsa on 8/24/2022.
//

#include "StructureAbandonedHouse.hpp"

#include "Minecraft/World/Chunk/Chunk.hpp"
#include "Minecraft/World/MinecraftWorld.hpp"
#include "Utility/Logger.hpp"

void
StructureAbandonedHouse::Generate( struct WorldChunk& chunk )
{
    const WorldChunk& startingChunk      = GetStartingChunk( chunk );
    const auto        startingCoordinate = startingChunk.WorldToChunkRelativeCoordinate( m_StartingPosition );
    const auto        startingIndex      = Structure::ToHorizontalIndex( startingCoordinate );
    const auto        heightAtPoint      = startingChunk.GetHeight( startingIndex, eNoiseHeight ) + 2;

    FillCubeHollow( chunk, BlockID::BirchPlanks,
                    GetMinecraftX( m_StartingPosition ), GetMinecraftX( m_StartingPosition ) + 7,
                    heightAtPoint, heightAtPoint + 5,
                    GetMinecraftZ( m_StartingPosition ), GetMinecraftZ( m_StartingPosition ) + 5, true );
    SetBlockWorld( chunk, m_StartingPosition + MakeMinecraftCoordinate( 1, heightAtPoint + 1, 1 ), BlockID::Barrel );
    SetBlockWorld( chunk, m_StartingPosition + MakeMinecraftCoordinate( 1, heightAtPoint + 1, 2 ), BlockID::Barrel );
    SetBlockWorld( chunk, m_StartingPosition + MakeMinecraftCoordinate( 1, heightAtPoint + 2, 1 ), BlockID::CraftingTable );

    FillFace<MinecraftCoordinateXIndex, MinecraftCoordinateYIndex>( chunk, BlockID::Air,
                                                                    GetMinecraftX( m_StartingPosition ) + 3, GetMinecraftX( m_StartingPosition ) + 4,
                                                                    heightAtPoint + 1, heightAtPoint + 3, GetMinecraftZ( m_StartingPosition ) + 5 );
}

bool
StructureAbandonedHouse::TryGenerate( struct WorldChunk& chunk, std::vector<std::shared_ptr<Structure>>& structureList )
{
    MinecraftNoise chunkNoise = chunk.CopyChunkNoise( );
    if ( ( chunkNoise.NextUint64( ) >> 1 ) % 10 != 0 ) return false;

    const auto startingX = chunkNoise.NextUint64( ) % SectionUnitLength;
    const auto startingZ = chunkNoise.NextUint64( ) % SectionUnitLength;

    auto chunkPosition = chunk.GetWorldCoordinate( );
    auto newStructure  = std::make_shared<StructureAbandonedHouse>( );

    GetMinecraftX( newStructure->m_StartingPosition ) = GetMinecraftX( chunkPosition ) += startingX;
    GetMinecraftZ( newStructure->m_StartingPosition ) = GetMinecraftZ( chunkPosition ) += startingZ;

    newStructure->AddPiece( { (float) GetMinecraftX( chunkPosition ), (float) GetMinecraftZ( chunkPosition ), GetMinecraftX( chunkPosition ) + 10.0f, GetMinecraftZ( chunkPosition ) + 10.0f } );

    structureList.emplace_back( std::move( newStructure ) );

    return true;
}
