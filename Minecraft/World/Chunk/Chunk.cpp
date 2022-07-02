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
    m_WorldHeightMap              = new uint32_t[ SectionSurfaceSize ];

    int      heightMapIndex = 0;
    uint32_t highestPoint   = 0;
    for ( int i = 0; i < SectionUnitLength; ++i )
        for ( int j = 0; j < SectionUnitLength; ++j )
        {
            m_WorldHeightMap[ heightMapIndex ] = int( std::lerp( ChunkMaxHeight * 0.1f, ChunkMaxHeight * 0.2f, worldPercentageHeightMap[ heightMapIndex ] ) );
            highestPoint                       = std::max( highestPoint, m_WorldHeightMap[ heightMapIndex ] );

            ++heightMapIndex;
        }

    m_SectionCount  = std::ceil( (float) highestPoint / SectionUnitLength );
    m_SectionHeight = m_SectionCount * SectionUnitLength;
    m_Blocks        = new Block[ m_SectionCount * SectionVolume ];
    m_BlockFaces    = new uint8_t[ m_SectionCount * SectionVolume ];

    auto blocksPtr = m_Blocks;
    for ( int i = 0; i < m_SectionHeight; ++i )
    {
        heightMapIndex = 0;
        for ( int k = 0; k < SectionUnitLength; ++k )
            for ( int j = 0; j < SectionUnitLength; ++j )
            {
                blocksPtr[ heightMapIndex ] = m_WorldHeightMap[ heightMapIndex ] < i ? BlockID::Air : BlockID::Stone;
                // blocksPtr[ heightMapIndex ] = BlockID::Air;
                ++heightMapIndex;
            }

        blocksPtr += SectionSurfaceSize;
    }

    //    if ( ManhattanDistance( MakeCoordinate( 0, 0, 0 ) ) == 0 )
    //    {
    //        for ( int i = 0; i < 16; ++i ) m_Blocks[ i ] = BlockID::Stone;
    //    }

    // RegenerateVisibleFaces( );
}

std::unique_ptr<float[]>
Chunk::GenerateHeightMap( const MinecraftNoise& generator )
{
    auto heightMap    = std::make_unique<float[]>( SectionSurfaceSize );
    auto heightMapPtr = heightMap.get( );

    auto chunkX = GetMinecraftX( m_Coordinate ) << SectionUnitLengthBinaryOffset;
    auto chunkZ = GetMinecraftZ( m_Coordinate ) << SectionUnitLengthBinaryOffset;

    for ( int i = 0; i < SectionUnitLength; ++i )
    {
        for ( int j = 0; j < SectionUnitLength; ++j )
        {
            *( heightMapPtr++ ) = generator.GetNoiseInt( chunkX + j, chunkZ + i );
        }
    }

    return std::move( heightMap );
}

void
Chunk::RegenerateVisibleFaces( )
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

    // Logger::getInstance( ).LogLine( ( ( ( MaxSectionInChunk << SectionVolumeBinaryOffset ) - 1 ) >> SectionSurfaceSizeBinaryOffset ), ( ( MaxSectionInChunk << SectionUnitLengthBinaryOffset ) - 1 ) );

    const auto RightChunk       = m_NearChunks[ DirRight ];
    const auto RightChunkHeight = ( RightChunk->m_SectionCount << SectionVolumeBinaryOffset ) - 1;
    const auto LeftChunk        = m_NearChunks[ DirLeft ];
    const auto LeftChunkHeight  = ( LeftChunk->m_SectionCount << SectionVolumeBinaryOffset ) - 1;
    const auto FrontChunk       = m_NearChunks[ DirFront ];
    const auto FrontChunkHeight = ( FrontChunk->m_SectionCount << SectionVolumeBinaryOffset ) - 1;
    const auto BackChunk        = m_NearChunks[ DirBack ];
    const auto BackChunkHeight  = ( BackChunk->m_SectionCount << SectionVolumeBinaryOffset ) - 1;

    for ( int i = 0; i < ( m_SectionCount << SectionVolumeBinaryOffset ); ++i )
    {
        m_BlockFaces[ i ] = 0;
        if ( m_Blocks[ i ].Transparent( ) ) continue;

        if ( ( i >> SectionSurfaceSizeBinaryOffset ) == ( m_SectionCount << SectionUnitLengthBinaryOffset ) - 1 || m_Blocks[ i + dirUpFaceOffset ].Transparent( ) ) [[unlikely]]
            m_BlockFaces[ i ] = DirUpBit;
        if ( ( i >> SectionSurfaceSizeBinaryOffset ) == 0 || m_Blocks[ i + dirDownFaceOffset ].Transparent( ) ) [[unlikely]]
            m_BlockFaces[ i ] |= DirDownBit;

        if ( ( i % SectionSurfaceSize ) >= SectionSurfaceSize - SectionUnitLength ) [[unlikely]]
        {
            if ( i > RightChunkHeight || RightChunk->m_Blocks[ i + dirRightChunkFaceOffset ].Transparent( ) )
            {
                m_BlockFaces[ i ] |= DirRightBit;
            }

        } else if ( m_Blocks[ i + dirRightFaceOffset ].Transparent( ) )
        {
            m_BlockFaces[ i ] |= DirRightBit;
        }

        if ( ( i % SectionSurfaceSize ) < SectionUnitLength ) [[unlikely]]
        {
            if ( i > LeftChunkHeight || LeftChunk->m_Blocks[ i + dirLeftChunkFaceOffset ].Transparent( ) )
            {
                m_BlockFaces[ i ] |= DirLeftBit;
            }

        } else if ( m_Blocks[ i + dirLeftFaceOffset ].Transparent( ) )
        {
            m_BlockFaces[ i ] |= DirLeftBit;
        }

        if ( ( i % SectionUnitLength ) == SectionUnitLength - 1 ) [[unlikely]]
        {
            if ( i > FrontChunkHeight || FrontChunk->m_Blocks[ i + dirFrontChunkFaceOffset ].Transparent( ) )
            {
                m_BlockFaces[ i ] |= DirFrontBit;
            }

        } else if ( m_Blocks[ i + dirFrontFaceOffset ].Transparent( ) )
        {
            m_BlockFaces[ i ] |= DirFrontBit;
        }

        if ( ( i % SectionUnitLength ) == 0 ) [[unlikely]]
        {
            if ( i > BackChunkHeight || BackChunk->m_Blocks[ i + dirBackChunkFaceOffset ].Transparent( ) )
            {
                m_BlockFaces[ i ] |= DirBackBit;
            }

        } else if ( m_Blocks[ i + dirBackFaceOffset ].Transparent( ) )
        {
            m_BlockFaces[ i ] |= DirBackBit;
        }
    }

    m_VisibleFacesCount = 0;
    for ( int i = 0; i < m_SectionCount * SectionVolume; ++i )
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
