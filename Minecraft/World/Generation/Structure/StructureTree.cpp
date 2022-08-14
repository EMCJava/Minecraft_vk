//
// Created by loys on 7/26/22.
//

#include "StructureTree.hpp"

#include "Minecraft/World/Chunk/Chunk.hpp"
#include "Minecraft/World/MinecraftWorld.hpp"
#include "Utility/Logger.hpp"

namespace
{

auto
ToHorizontalIndex( const auto& coordinate )
{
    return ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( coordinate ) ) + GetMinecraftX( coordinate );
}
};   // namespace

void
StructureTree::Generate( Chunk& chunk )
{
    auto           startingCoordinate = chunk.WorldToChunkRelativeCoordinate( m_StartingPosition );
    auto           startingIndex      = ToHorizontalIndex( startingCoordinate );
    CoordinateType heightAtPoint, treeHeight;
    MinecraftNoise chunkNoise;

    if ( chunk.IsPointInsideHorizontally( m_StartingPosition ) )
    {
        heightAtPoint = chunk.GetHeight( startingIndex, eNoiseHeight ) + 1;
        treeHeight    = std::min( ChunkMaxHeight - heightAtPoint, 7 );
        chunkNoise    = chunk.GetChunkNoise( );

    } else
    {
        assert( GetMinecraftY( m_StartingPosition ) == 0 );
        const auto originalChunkCoordinate = m_StartingPosition >> SectionUnitLengthBinaryOffset;
        // const
        auto originalChunk           = chunk.GetWorld( )->GetChunkCache( originalChunkCoordinate );
        const auto relativeCoordinate      = originalChunk->WorldToChunkRelativeCoordinate( m_StartingPosition );
        if ( GetMinecraftX( relativeCoordinate ) < 0 || GetMinecraftX( relativeCoordinate ) >= SectionUnitLength || GetMinecraftZ( relativeCoordinate ) < 0 || GetMinecraftZ( relativeCoordinate ) >= SectionUnitLength )
            originalChunk = chunk.GetWorld( )->GetChunkCache( originalChunkCoordinate );
        const auto horizontalIndex = ToHorizontalIndex( relativeCoordinate );
        heightAtPoint              = originalChunk->GetHeight( horizontalIndex, eNoiseHeight ) + 1;
        treeHeight                 = std::min( ChunkMaxHeight - heightAtPoint, 7 );
        chunkNoise                 = originalChunk->GetChunkNoise( );
    }

    // trunk
    if ( chunk.IsPointInsideHorizontally( m_StartingPosition ) )
    {

        for ( int i = 0; i < treeHeight; ++i )
        {
            chunk.SetBlock( startingIndex + ScaleToSecond<1, SectionSurfaceSize>( heightAtPoint + i ), BlockID::AcaciaLog );
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
                    chunk.SetBlockAtWorldCoordinate( leafOrigin + MakeMinecraftCoordinate( x, y, z ), BlockID::AzaleaLeaves );
                }
            }
}

bool
StructureTree::TryGenerate( Chunk& chunk, std::vector<std::shared_ptr<Structure>>& structureList )
{
    bool           generated  = false;
    MinecraftNoise chunkNoise = chunk.GetChunkNoise( );

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
                GetMinecraftZ( newStructure->m_StartingPosition ) = GetMinecraftZ( chunkPosition ) += j;

                newStructure->AddPiece( { GetMinecraftX( chunkPosition ) - 4.0f, GetMinecraftZ( chunkPosition ) - 4.0f, GetMinecraftX( chunkPosition ) + 4.0f, GetMinecraftZ( chunkPosition ) + 4.0f } );

                structureList.emplace_back( std::move( newStructure ) );
            }
        }

    return generated;
}
