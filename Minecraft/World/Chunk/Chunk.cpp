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

    m_Blocks     = new Block[ ChunkVolume ];
    m_BlockFaces = new uint8_t[ ChunkVolume ];

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

    //    if ( ManhattanDistance( { 0, 0, 0 } ) != 0 )
    //        for ( int i = 0; i < ChunkVolume; ++i )
    //            m_Blocks[ i ] = BlockID::Air;
}

void
Chunk::RegenerateVisibleFaces( )
{
    for ( int i = 0; i < ChunkVolume; ++i )
        RegenerateVisibleFacesAt( i );

    m_VisibleFacesCount = 0;
    for ( int i = 0; i < ChunkVolume; ++i )
        m_VisibleFacesCount += std::popcount( m_BlockFaces[ i ] );
}

bool
Chunk::SyncChunkFromDirection( Chunk* other, Direction fromDir, bool changes )
{
    if ( m_EmptySlot == 0 )
    {
        if ( m_NearChunks[ fromDir ] == nullptr )
        {
            m_NearChunks[ fromDir ] = other;
        } else if ( changes )
        {
            // TODO: Update vertex buffer
        }
    } else
    {
        m_NearChunks[ fromDir ] = other;
        m_EmptySlot &= ~( 1 << fromDir );

        if ( m_EmptySlot == 0 )
        {
            // Chunk surrounding completed
            RegenerateVisibleFaces( );
            return true;
        }
    }

    return false;
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
    assert( blockIndex >= 0 && blockIndex < ChunkVolume );

    Logger::getInstance( ).LogLine( "Setting block at chunk:", m_Coordinate, "coordinate:", blockCoordinate, "index:", blockIndex, "from:", toString( (BlockID) m_Blocks[ blockIndex ] ), "to:", toString( (BlockID) block ) );

    if ( m_Blocks[ blockIndex ] == block ) return false;

    bool transparencyChanged = m_Blocks[ blockIndex ].Transparent( ) ^ block.Transparent( );
    m_Blocks[ blockIndex ]   = block;

    if ( transparencyChanged && m_EmptySlot == 0 )
    {
        static constexpr auto dirUpFaceOffset    = SectionSurfaceSize;
        static constexpr auto dirDownFaceOffset  = -SectionSurfaceSize;
        static constexpr auto dirRightFaceOffset = SectionUnitLength;
        static constexpr auto dirLeftFaceOffset  = -SectionUnitLength;
        static constexpr auto dirFrontFaceOffset = 1;
        static constexpr auto dirBackFaceOffset  = -1;

        const auto originalFaceCount = std::popcount( m_BlockFaces[ blockIndex ] );
        RegenerateVisibleFacesAt( blockIndex );
        const auto newFaceCount = std::popcount( m_BlockFaces[ blockIndex ] );

        int         faceDiff     = 0;
        const auto* blockPtr     = m_Blocks + blockIndex;
        auto* const blockFacePtr = m_BlockFaces + blockIndex;
        if ( GetMinecraftY( blockCoordinate ) != ChunkMaxHeight - 1 )
            if ( !blockPtr[ dirUpFaceOffset ].Transparent( ) )
            {
                blockFacePtr[ dirUpFaceOffset ] ^= DirDownBit;
                ++faceDiff;
            }

        if ( GetMinecraftY( blockCoordinate ) != 0 )
            if ( !blockPtr[ dirDownFaceOffset ].Transparent( ) )
            {
                blockFacePtr[ dirDownFaceOffset ] ^= DirUpBit;
                ++faceDiff;
            }

        if ( GetMinecraftZ( blockCoordinate ) != SectionUnitLength - 1 )
            if ( !blockPtr[ dirRightFaceOffset ].Transparent( ) )
            {
                blockFacePtr[ dirRightFaceOffset ] ^= DirLeftBit;
                ++faceDiff;
            }

        if ( GetMinecraftZ( blockCoordinate ) != 0 )
            if ( !blockPtr[ dirLeftFaceOffset ].Transparent( ) )
            {
                blockFacePtr[ dirLeftFaceOffset ] ^= DirRightBit;
                ++faceDiff;
            }

        if ( GetMinecraftX( blockCoordinate ) != SectionUnitLength - 1 )
            if ( !blockPtr[ dirFrontFaceOffset ].Transparent( ) )
            {
                blockFacePtr[ dirFrontFaceOffset ] ^= DirBackBit;
                ++faceDiff;
            }

        if ( GetMinecraftX( blockCoordinate ) != 0 )
            if ( !blockPtr[ dirBackFaceOffset ].Transparent( ) )
            {
                blockFacePtr[ dirBackFaceOffset ] ^= DirFrontBit;
                ++faceDiff;
            }

        if ( block.Transparent( ) )
        {
            m_VisibleFacesCount += faceDiff - originalFaceCount;
        } else
        {
            m_VisibleFacesCount -= faceDiff + newFaceCount;
        }

#ifndef NDEBUG

        int checkingFaceCount = 0;
        for ( int i = 0; i < ChunkVolume; ++i )
            checkingFaceCount += std::popcount( m_BlockFaces[ i ] );
        assert( m_VisibleFacesCount == checkingFaceCount );

#endif
    }

    return true;
}

void
Chunk::RegenerateVisibleFacesAt( uint32_t index )
{
    static constexpr auto dirUpFaceOffset         = SectionSurfaceSize;
    static constexpr auto dirDownFaceOffset       = -SectionSurfaceSize;
    static constexpr auto dirRightFaceOffset      = SectionUnitLength;
    static constexpr auto dirRightChunkFaceOffset = SectionUnitLength - SectionSurfaceSize;
    static constexpr auto dirLeftFaceOffset       = -SectionUnitLength;
    static constexpr auto dirLeftChunkFaceOffset  = SectionSurfaceSize - SectionUnitLength;
    static constexpr auto dirFrontFaceOffset      = 1;
    static constexpr auto dirFrontChunkFaceOffset = 1 - SectionUnitLength;
    static constexpr auto dirBackFaceOffset       = -1;
    static constexpr auto dirBackChunkFaceOffset  = SectionUnitLength - 1;

    assert( m_EmptySlot == 0 );
    assert( m_NearChunks[ DirRight ] != nullptr && m_NearChunks[ DirLeft ] != nullptr && m_NearChunks[ DirFront ] != nullptr && m_NearChunks[ DirBack ] != nullptr );

    m_BlockFaces[ index ] = 0;
    if ( m_Blocks[ index ].Transparent( ) ) return;

    if ( ( index >> SectionSurfaceSizeBinaryOffset ) == ChunkMaxHeight - 1 || m_Blocks[ index + dirUpFaceOffset ].Transparent( ) ) [[unlikely]]
        m_BlockFaces[ index ] = DirUpBit;
    if ( ( index >> SectionSurfaceSizeBinaryOffset ) == 0 || m_Blocks[ index + dirDownFaceOffset ].Transparent( ) ) [[unlikely]]
        m_BlockFaces[ index ] |= DirDownBit;

    if ( ( index % SectionSurfaceSize ) >= SectionSurfaceSize - SectionUnitLength ) [[unlikely]]
    {
        if ( m_NearChunks[ DirRight ]->m_Blocks[ index + dirRightChunkFaceOffset ].Transparent( ) )
        {
            m_BlockFaces[ index ] |= DirRightBit;
        }

    } else if ( m_Blocks[ index + dirRightFaceOffset ].Transparent( ) )
    {
        m_BlockFaces[ index ] |= DirRightBit;
    }

    if ( ( index % SectionSurfaceSize ) < SectionUnitLength ) [[unlikely]]
    {
        if ( m_NearChunks[ DirLeft ]->m_Blocks[ index + dirLeftChunkFaceOffset ].Transparent( ) )
        {
            m_BlockFaces[ index ] |= DirLeftBit;
        }

    } else if ( m_Blocks[ index + dirLeftFaceOffset ].Transparent( ) )
    {
        m_BlockFaces[ index ] |= DirLeftBit;
    }

    if ( ( index % SectionUnitLength ) == SectionUnitLength - 1 ) [[unlikely]]
    {
        if ( m_NearChunks[ DirFront ]->m_Blocks[ index + dirFrontChunkFaceOffset ].Transparent( ) )
        {
            m_BlockFaces[ index ] |= DirFrontBit;
        }

    } else if ( m_Blocks[ index + dirFrontFaceOffset ].Transparent( ) )
    {
        m_BlockFaces[ index ] |= DirFrontBit;
    }

    if ( ( index % SectionUnitLength ) == 0 ) [[unlikely]]
    {
        if ( m_NearChunks[ DirBack ]->m_Blocks[ index + dirBackChunkFaceOffset ].Transparent( ) )
        {
            m_BlockFaces[ index ] |= DirBackBit;
        }

    } else if ( m_Blocks[ index + dirBackFaceOffset ].Transparent( ) )
    {
        m_BlockFaces[ index ] |= DirBackBit;
    }
}
