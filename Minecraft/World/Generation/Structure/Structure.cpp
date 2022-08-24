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
Structure::SetBlock( Chunk& chunk, const BlockCoordinate& worldCoordinate, const Block& block, bool replace )
{
    return chunk.SetBlockAtWorldCoordinate( worldCoordinate, block, replace );
}

WorldChunk&
Structure::GetStartingChunk( WorldChunk& generatingChunk )
{

    if ( generatingChunk.IsPointInsideHorizontally( m_StartingPosition ) ) return generatingChunk;

    const auto originalChunkCoordinate = m_StartingPosition >> SectionUnitLengthBinaryOffset;
    const auto originalChunk           = MinecraftServer::GetInstance( ).GetWorld( ).GetChunkCache( originalChunkCoordinate );
    const auto relativeCoordinate      = originalChunk->WorldToChunkRelativeCoordinate( m_StartingPosition );
    assert( !( GetMinecraftX( relativeCoordinate ) < 0 || GetMinecraftX( relativeCoordinate ) >= SectionUnitLength || GetMinecraftZ( relativeCoordinate ) < 0 || GetMinecraftZ( relativeCoordinate ) >= SectionUnitLength ) );

    return *originalChunk;
}
