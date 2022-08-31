//
// Created by loys on 5/23/22.
//

#include "Chunk.hpp"
#include <Minecraft/World/Generation/Structure/StructureTree.hpp>

#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>

#include <cmath>

const Block*
Chunk::CheckBlock( const BlockCoordinate& blockCoordinate ) const
{
    if ( GetMinecraftY( blockCoordinate ) >= ChunkMaxHeight || GetMinecraftY( blockCoordinate ) < 0 ) return nullptr;
    const auto& blockIndex = ScaleToSecond<1, SectionSurfaceSize>( GetMinecraftY( blockCoordinate ) ) + ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( blockCoordinate ) ) + GetMinecraftX( blockCoordinate );
    return &At( blockIndex );
}

Block*
Chunk::GetBlock( const BlockCoordinate& blockCoordinate )
{
    if ( GetMinecraftY( blockCoordinate ) >= ChunkMaxHeight || GetMinecraftY( blockCoordinate ) < 0 ) return nullptr;
    const auto& blockIndex = ScaleToSecond<1, SectionSurfaceSize>( GetMinecraftY( blockCoordinate ) ) + ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( blockCoordinate ) ) + GetMinecraftX( blockCoordinate );
    return &At( blockIndex );
}

bool
Chunk::SetBlock( const BlockCoordinate& blockCoordinate, const Block& block )
{
    assert( m_Blocks != nullptr );
    const auto& blockIndex = GetBlockIndex( blockCoordinate );
    return SetBlock( blockIndex, block );
}

bool
Chunk::SetBlock( const uint32_t& blockIndex, const Block& block )
{
    assert( m_Blocks != nullptr && m_HeightMap != nullptr );
    assert( blockIndex >= 0 && blockIndex < ChunkVolume );

    // Logger::getInstance( ).LogLine( Logger::LogType::eVerbose, "Setting block at chunk:", m_Coordinate, "index:", blockIndex, "from:", toString( (BlockID) m_Blocks[ blockIndex ] ), "to:", toString( (BlockID) block ) );

    if ( At( blockIndex ) == block ) return false;

    // update height map
    if ( block != Air )
    {
        auto&      originalHeight = m_HeightMap[ blockIndex & GetConstantBinaryMask<SectionSurfaceSize>( ) ];
        const auto height         = blockIndex >> SectionSurfaceSizeBinaryOffset;
        if ( originalHeight < height ) originalHeight = height;
    } else
    {
        const auto horizontalIndex = blockIndex & GetConstantBinaryMask<SectionSurfaceSize>( );
        auto&      originalHeight  = m_HeightMap[ horizontalIndex ];
        const auto height          = blockIndex >> SectionSurfaceSizeBinaryOffset;

        // remove the highest block
        if ( originalHeight == height )
        {
            int i = originalHeight - 1;
            while ( At( blockIndex - ScaleToSecond<1, SectionSurfaceSize>( i ) ) != Air && --i >= 0 ) { }
            originalHeight = i;
        }
    }

    At( blockIndex ) = block;
    return true;
}

CoordinateType
Chunk::GetHeight( uint32_t index )
{
    return m_HeightMap[ index ];
}

void
Chunk::SetCoordinate( const ChunkCoordinate& coordinate )
{
    m_WorldCoordinate = m_Coordinate = coordinate;
    GetMinecraftX( m_WorldCoordinate ) <<= SectionUnitLengthBinaryOffset;
    GetMinecraftZ( m_WorldCoordinate ) <<= SectionUnitLengthBinaryOffset;

    minX = GetMinecraftX( m_WorldCoordinate );
    maxX = GetMinecraftX( m_WorldCoordinate ) + SectionUnitLength;
    minY = GetMinecraftZ( m_WorldCoordinate );
    maxY = GetMinecraftZ( m_WorldCoordinate ) + SectionUnitLength;
}

bool
Chunk::SetBlockAtWorldCoordinate( const BlockCoordinate& blockCoordinate, const Block& block, bool replace )
{
    if ( !IsPointInside( blockCoordinate ) ) return false;

    const auto index = GetBlockIndex( blockCoordinate - m_WorldCoordinate );
    if ( replace || At( index ) == Air )
        return SetBlock( index, block );

    return false;
}