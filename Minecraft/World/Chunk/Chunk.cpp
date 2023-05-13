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
    return &At( GetBlockIndex( blockCoordinate ) );
}

Block*
Chunk::GetBlock( const BlockCoordinate& blockCoordinate )
{
    if ( GetMinecraftY( blockCoordinate ) >= ChunkMaxHeight || GetMinecraftY( blockCoordinate ) < 0 ) return nullptr;
    return &At( GetBlockIndex( blockCoordinate ) );
}

bool
Chunk::SetBlock( const BlockCoordinate& blockCoordinate, const Block& block, bool replace )
{
    assert( m_Blocks != nullptr );
    const auto& blockIndex = GetBlockIndex( blockCoordinate );
    return SetBlock( blockIndex, block, replace );
}

bool
Chunk::SetBlock( const uint32_t& blockIndex, const Block& block, bool replace )
{
    assert( m_Blocks != nullptr && m_HeightMap != nullptr );
    assert( blockIndex >= 0 && blockIndex < ChunkVolume );

    // Logger::getInstance( ).LogLine( Logger::LogType::eVerbose, "Setting block at chunk:", m_Position, "index:", blockIndex, "from:", toString( (BlockID) m_Blocks[ blockIndex ] ), "to:", toString( (BlockID) block ) );

    if ( At( blockIndex ) == block ) return false;             // same block
    if ( !replace && At( blockIndex ) != Air ) return false;   // not replacing

    // update height map
    if ( block != Air )
    {
        auto&      originalHeight = m_HeightMap[ blockIndex & GetConstantBinaryMask<SectionSurfaceSize>( ) ];
        const auto height         = static_cast<int32_t>( blockIndex >> SectionSurfaceSizeBinaryOffset );
        if ( originalHeight < height ) originalHeight = height;
    } else
    {
        const auto horizontalIndex = blockIndex & GetConstantBinaryMask<SectionSurfaceSize>( );
        auto&      originalHeight  = m_HeightMap[ horizontalIndex ];
        const auto height          = static_cast<int32_t>( blockIndex >> SectionSurfaceSizeBinaryOffset );

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
    m_Coordinate      = coordinate;
    m_WorldCoordinate = ToMinecraftCoordinate( m_Coordinate << SectionUnitLengthBinaryOffset );

    minX = (FloatTy) GetMinecraftX( m_WorldCoordinate );
    maxX = (FloatTy) ( GetMinecraftX( m_WorldCoordinate ) + SectionUnitLength );
    minY = (FloatTy) GetMinecraftZ( m_WorldCoordinate );
    maxY = (FloatTy) ( GetMinecraftZ( m_WorldCoordinate ) + SectionUnitLength );
}

bool
Chunk::SetBlockAtWorldCoordinate( const BlockCoordinate& blockCoordinate, const Block& block, bool replace )
{
    if ( !IsPointInside( blockCoordinate ) ) return false;
    return SetBlock( GetBlockIndex( blockCoordinate - m_WorldCoordinate ), block, replace );
}

size_t
Chunk::GetObjectSize( ) const
{
    return sizeof( Chunk ) + ( m_Blocks ? sizeof( m_Blocks[ 0 ] ) * ChunkVolume : 0 ) + ( m_HeightMap ? sizeof( m_HeightMap[ 0 ] ) * SectionSurfaceSize : 0 );
}
