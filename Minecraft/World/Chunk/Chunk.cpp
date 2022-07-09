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

    const auto RightChunk = m_NearChunks[ DirRight ];
    const auto LeftChunk  = m_NearChunks[ DirLeft ];
    const auto FrontChunk = m_NearChunks[ DirFront ];
    const auto BackChunk  = m_NearChunks[ DirBack ];

    for ( int i = 0; i < ChunkVolume; ++i )
    {
        m_BlockFaces[ i ] = 0;
        if ( m_Blocks[ i ].Transparent( ) ) continue;

        if ( ( i >> SectionSurfaceSizeBinaryOffset ) == ChunkMaxHeight - 1 || m_Blocks[ i + dirUpFaceOffset ].Transparent( ) ) [[unlikely]]
            m_BlockFaces[ i ] = DirUpBit;
        if ( ( i >> SectionSurfaceSizeBinaryOffset ) == 0 || m_Blocks[ i + dirDownFaceOffset ].Transparent( ) ) [[unlikely]]
            m_BlockFaces[ i ] |= DirDownBit;

        if ( ( i % SectionSurfaceSize ) >= SectionSurfaceSize - SectionUnitLength ) [[unlikely]]
        {
            if ( RightChunk->m_Blocks[ i + dirRightChunkFaceOffset ].Transparent( ) )
            {
                m_BlockFaces[ i ] |= DirRightBit;
            }

        } else if ( m_Blocks[ i + dirRightFaceOffset ].Transparent( ) )
        {
            m_BlockFaces[ i ] |= DirRightBit;
        }

        if ( ( i % SectionSurfaceSize ) < SectionUnitLength ) [[unlikely]]
        {
            if ( LeftChunk->m_Blocks[ i + dirLeftChunkFaceOffset ].Transparent( ) )
            {
                m_BlockFaces[ i ] |= DirLeftBit;
            }

        } else if ( m_Blocks[ i + dirLeftFaceOffset ].Transparent( ) )
        {
            m_BlockFaces[ i ] |= DirLeftBit;
        }

        if ( ( i % SectionUnitLength ) == SectionUnitLength - 1 ) [[unlikely]]
        {
            if ( FrontChunk->m_Blocks[ i + dirFrontChunkFaceOffset ].Transparent( ) )
            {
                m_BlockFaces[ i ] |= DirFrontBit;
            }

        } else if ( m_Blocks[ i + dirFrontFaceOffset ].Transparent( ) )
        {
            m_BlockFaces[ i ] |= DirFrontBit;
        }

        if ( ( i % SectionUnitLength ) == 0 ) [[unlikely]]
        {
            if ( BackChunk->m_Blocks[ i + dirBackChunkFaceOffset ].Transparent( ) )
            {
                m_BlockFaces[ i ] |= DirBackBit;
            }

        } else if ( m_Blocks[ i + dirBackFaceOffset ].Transparent( ) )
        {
            m_BlockFaces[ i ] |= DirBackBit;
        }
    }

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

    const auto noiseOffset = MinecraftServer::GetInstance( ).GetWorld( ).GetTerrainNoiseOffset( );

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
