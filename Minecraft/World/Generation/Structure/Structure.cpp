//
// Created by samsa on 8/15/2022.
//

#include "Structure.hpp"

#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>
#include <Minecraft/World/Chunk/WorldChunk.hpp>

namespace
{

auto
ToHorizontalIndex( const auto& coordinate )
{
    return ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( coordinate ) ) + GetMinecraftX( coordinate );
}
};   // namespace

bool
Structure::SetBlockWorld( Chunk& chunk, const BlockCoordinate& worldCoordinate, const Block& block, bool replace )
{
    return chunk.SetBlockAtWorldCoordinate( worldCoordinate, block, replace );
}

WorldChunk&
Structure::GetStartingChunk( WorldChunk& generatingChunk )
{

    if ( generatingChunk.IsPointInsideHorizontally( m_StartingPosition ) ) return generatingChunk;

    const auto originalChunkCoordinate = m_StartingPosition >> SectionUnitLengthBinaryOffset;
    const auto originalChunk           = MinecraftServer::GetInstance( ).GetWorld( ).GetChunkCacheUnsafe( originalChunkCoordinate );
    const auto relativeCoordinate      = originalChunk->WorldToChunkRelativeCoordinate( m_StartingPosition );
    assert( !( GetMinecraftX( relativeCoordinate ) < 0 || GetMinecraftX( relativeCoordinate ) >= SectionUnitLength || GetMinecraftZ( relativeCoordinate ) < 0 || GetMinecraftZ( relativeCoordinate ) >= SectionUnitLength ) );

    return *originalChunk;
}

void
Structure::FillCubeHollow( Chunk& chunk, const Block& block, CoordinateType minX, CoordinateType maxX, CoordinateType minY, CoordinateType maxY, CoordinateType minZ, CoordinateType maxZ, bool replace )
{
    FillFace<MinecraftCoordinateXIndex, MinecraftCoordinateYIndex>( chunk, block, minX, maxX, minY, maxY, minZ, replace );
    FillFace<MinecraftCoordinateXIndex, MinecraftCoordinateZIndex>( chunk, block, minX, maxX, minZ + 1, maxZ, minY, replace );
    FillFace<MinecraftCoordinateYIndex, MinecraftCoordinateZIndex>( chunk, block, minY + 1, maxY, minZ + 1, maxZ, minX, replace );
    FillFace<MinecraftCoordinateYIndex, MinecraftCoordinateZIndex>( chunk, block, minY + 1, maxY, minZ + 1, maxZ, maxX, replace );
    if ( maxX - minX > 1 )
    {
        FillFace<MinecraftCoordinateXIndex, MinecraftCoordinateYIndex>( chunk, block, minX + 1, maxX - 1, minY + 1, maxY, maxZ, replace );
        if ( maxZ - minZ > 1 ) FillFace<MinecraftCoordinateXIndex, MinecraftCoordinateZIndex>( chunk, block, minX + 1, maxX - 1, minZ + 1, maxZ - 1, maxY, replace );
    }
}

void
Structure::FillCube( Chunk& chunk, const Block& block, CoordinateType minX, CoordinateType maxX, CoordinateType minY, CoordinateType maxY, CoordinateType minZ, CoordinateType maxZ, bool replace )
{
    BlockCoordinate currentCoordinate = MakeMinecraftCoordinate( minX, minY, minZ );
    for ( int i = minX; i <= maxX; i++ )
    {
        std::get<MinecraftCoordinateXIndex>( currentCoordinate ) = i;
        for ( int j = minY; j <= maxY; j++ )
        {
            std::get<MinecraftCoordinateYIndex>( currentCoordinate ) = j;
            for ( int k = minZ; k <= maxZ; k++ )
            {
                std::get<MinecraftCoordinateZIndex>( currentCoordinate ) = k;
                SetBlockWorld( chunk, currentCoordinate, block, replace );
            }
        }
    }
}
