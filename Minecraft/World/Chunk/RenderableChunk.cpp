//
// Created by loys on 5/24/22.
//

#include <Minecraft/Application/MainApplication.hpp>
#include <Minecraft/util/MinecraftConstants.hpp>

#include <Graphic/Vulkan/VulkanAPI.hpp>

#include <Utility/Logger.hpp>

#include "RenderableChunk.hpp"

namespace
{

constexpr auto dirUpFaceOffset         = SectionSurfaceSize;
constexpr auto dirDownFaceOffset       = -SectionSurfaceSize;
constexpr auto dirRightFaceOffset      = SectionUnitLength;
constexpr auto dirRightChunkFaceOffset = SectionUnitLength - SectionSurfaceSize;
constexpr auto dirLeftFaceOffset       = -SectionUnitLength;
constexpr auto dirLeftChunkFaceOffset  = SectionSurfaceSize - SectionUnitLength;
constexpr auto dirFrontFaceOffset      = 1;
constexpr auto dirFrontChunkFaceOffset = 1 - SectionUnitLength;
constexpr auto dirBackFaceOffset       = -1;
constexpr auto dirBackChunkFaceOffset  = SectionUnitLength - 1;
}   // namespace

void
RenderableChunk::ResetRenderBuffer( )
{
    DeleteCache( );
    m_NeighborTransparency = new uint32_t[ ChunkVolume ];
}

void
RenderableChunk::RegenerateVisibleFaces( )
{
    std::lock_guard<std::recursive_mutex> lock( m_SyncMutex );

    for ( int i = 0; i < ChunkVolume; ++i )
        UpdateNeighborAt( i );

    m_VisibleFacesCount = 0;
    for ( int i = 0; i < ChunkVolume; ++i )
        // only count faces directed covered
        m_VisibleFacesCount += std::popcount( m_NeighborTransparency[ i ] & DirFaceMask );
}

void
RenderableChunk::UpdateNeighborAt( const uint32_t index )
{
    assert( m_EmptySlot == 0 );
    assert( std::all_of( m_NearChunks.begin( ), m_NearChunks.end( ), []( const auto& chunk ) { return chunk != nullptr; } ) );
    assert( index >= 0 && index < ChunkVolume );
    assert( m_NeighborTransparency != nullptr );

    // TODO: use zero to reset
    auto& blockNeighborTransparency = m_NeighborTransparency[ index ] = DirFrontRightBit | DirBackRightBit | DirFrontLeftBit | DirBackLeftBit
        | DirFrontRightUpBit
        | DirBackRightUpBit
        | DirFrontLeftUpBit
        | DirBackLeftUpBit
        | DirFrontRightDownBit
        | DirBackRightDownBit
        | DirFrontLeftDownBit
        | DirBackLeftDownBit;

    if ( At( index ).Transparent( ) ) return;

    const bool blockOnTop       = indexToHeight( index ) == ChunkMaxHeight - 1;
    const bool blockOnBottom    = indexToHeight( index ) == 0;
    const bool blockAtMostRight = ( index % SectionSurfaceSize ) >= SectionSurfaceSize - SectionUnitLength;
    const bool blockAtMostLeft  = ( index % SectionSurfaceSize ) < SectionUnitLength;
    const bool blockAtMostFront = ( index % SectionUnitLength ) == SectionUnitLength - 1;
    const bool blockAtMostBack  = ( index % SectionUnitLength ) == 0;

    if ( blockOnTop ) [[unlikely]]
        // no block can be on top
        blockNeighborTransparency |= DirUpBit | DirFrontUpBit | DirBackUpBit | DirRightUpBit | DirLeftUpBit | DirFrontRightUpBit | DirBackRightUpBit | DirFrontLeftUpBit | DirBackLeftUpBit;
    else if ( At( index + dirUpFaceOffset ).Transparent( ) )
        blockNeighborTransparency |= DirUpBit;


    if ( blockOnBottom ) [[unlikely]]
        // no block can be bellow bottom
        blockNeighborTransparency |= DirDownBit | DirFrontDownBit | DirBackDownBit | DirRightDownBit | DirLeftDownBit | DirFrontRightDownBit | DirBackRightDownBit | DirFrontLeftDownBit | DirBackLeftDownBit;
    else if ( At( index + dirDownFaceOffset ).Transparent( ) )
        blockNeighborTransparency |= DirDownBit;

    std::array<std::pair<RenderableChunk*, uint32_t>, DirHorizontalSize> m_HorizontalIndexChunkAfterPointMoved;
    if ( blockAtMostRight ) [[unlikely]]
    {
        m_HorizontalIndexChunkAfterPointMoved[ DirRight ] = { m_NearChunks[ DirRight ], index + dirRightChunkFaceOffset };
        if ( m_NearChunks[ DirRight ]->At( index + dirRightChunkFaceOffset ).Transparent( ) )
        {
            blockNeighborTransparency |= DirRightBit;
        }

    } else
    {
        m_HorizontalIndexChunkAfterPointMoved[ DirRight ] = { this, index + dirRightFaceOffset };
        if ( At( index + dirRightFaceOffset ).Transparent( ) )
            blockNeighborTransparency |= DirRightBit;
    }

    if ( blockAtMostLeft ) [[unlikely]]
    {
        m_HorizontalIndexChunkAfterPointMoved[ DirLeft ] = { m_NearChunks[ DirLeft ], index + dirLeftChunkFaceOffset };
        if ( m_NearChunks[ DirLeft ]->At( index + dirLeftChunkFaceOffset ).Transparent( ) )
        {
            blockNeighborTransparency |= DirLeftBit;
        }

    } else
    {
        m_HorizontalIndexChunkAfterPointMoved[ DirLeft ] = { this, index + dirLeftFaceOffset };
        if ( At( index + dirLeftFaceOffset ).Transparent( ) )
            blockNeighborTransparency |= DirLeftBit;
    }

    if ( blockAtMostFront ) [[unlikely]]
    {
        m_HorizontalIndexChunkAfterPointMoved[ DirFront ] = { m_NearChunks[ DirFront ], index + dirFrontChunkFaceOffset };
        if ( m_NearChunks[ DirFront ]->At( index + dirFrontChunkFaceOffset ).Transparent( ) )
        {
            blockNeighborTransparency |= DirFrontBit;
        }

    } else
    {
        m_HorizontalIndexChunkAfterPointMoved[ DirFront ] = { this, index + dirFrontFaceOffset };
        if ( At( index + dirFrontFaceOffset ).Transparent( ) )
            blockNeighborTransparency |= DirFrontBit;
    }

    if ( blockAtMostBack ) [[unlikely]]
    {
        m_HorizontalIndexChunkAfterPointMoved[ DirBack ] = { m_NearChunks[ DirBack ], index + dirBackChunkFaceOffset };
        if ( m_NearChunks[ DirBack ]->At( index + dirBackChunkFaceOffset ).Transparent( ) )
        {
            blockNeighborTransparency |= DirBackBit;
        }

    } else
    {
        m_HorizontalIndexChunkAfterPointMoved[ DirBack ] = { this, index + dirBackFaceOffset };
        if ( At( index + dirBackFaceOffset ).Transparent( ) )
            blockNeighborTransparency |= DirBackBit;
    }

    if ( !blockOnTop )
    {
        if ( m_HorizontalIndexChunkAfterPointMoved[ DirFront ].first->At( m_HorizontalIndexChunkAfterPointMoved[ DirFront ].second + dirUpFaceOffset ).Transparent( ) )
        {
            blockNeighborTransparency |= DirFrontUpBit;
        }

        if ( m_HorizontalIndexChunkAfterPointMoved[ DirBack ].first->At( m_HorizontalIndexChunkAfterPointMoved[ DirBack ].second + dirUpFaceOffset ).Transparent( ) )
        {
            blockNeighborTransparency |= DirBackUpBit;
        }

        if ( m_HorizontalIndexChunkAfterPointMoved[ DirRight ].first->At( m_HorizontalIndexChunkAfterPointMoved[ DirRight ].second + dirUpFaceOffset ).Transparent( ) )
        {
            blockNeighborTransparency |= DirRightUpBit;
        }

        if ( m_HorizontalIndexChunkAfterPointMoved[ DirLeft ].first->At( m_HorizontalIndexChunkAfterPointMoved[ DirLeft ].second + dirUpFaceOffset ).Transparent( ) )
        {
            blockNeighborTransparency |= DirLeftUpBit;
        }
    }

    if ( !blockOnBottom )
    {
        if ( m_HorizontalIndexChunkAfterPointMoved[ DirFront ].first->At( m_HorizontalIndexChunkAfterPointMoved[ DirFront ].second + dirDownFaceOffset ).Transparent( ) )
        {
            blockNeighborTransparency |= DirFrontDownBit;
        }

        if ( m_HorizontalIndexChunkAfterPointMoved[ DirBack ].first->At( m_HorizontalIndexChunkAfterPointMoved[ DirBack ].second + dirDownFaceOffset ).Transparent( ) )
        {
            blockNeighborTransparency |= DirBackDownBit;
        }

        if ( m_HorizontalIndexChunkAfterPointMoved[ DirRight ].first->At( m_HorizontalIndexChunkAfterPointMoved[ DirRight ].second + dirDownFaceOffset ).Transparent( ) )
        {
            blockNeighborTransparency |= DirRightDownBit;
        }

        if ( m_HorizontalIndexChunkAfterPointMoved[ DirLeft ].first->At( m_HorizontalIndexChunkAfterPointMoved[ DirLeft ].second + dirDownFaceOffset ).Transparent( ) )
        {
            blockNeighborTransparency |= DirLeftDownBit;
        }
    }
}


void
RenderableChunk::GenerateRenderBuffer( )
{
    if ( m_VisibleFacesCount == 0 ) return;

    // Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Generating chunk:", chunk.GetCoordinate( ) );

    auto [ chunkX, chunkZ, chunkY ] = GetChunkCoordinate( );

    chunkX <<= SectionUnitLengthBinaryOffset;
    chunkZ <<= SectionUnitLengthBinaryOffset;

    static const std::array<IndexBufferType, FaceIndicesCount> blockIndices = { 0, 1, 2, 2, 3, 0 };

    const auto verticesDataSize = ScaleToSecond<1, sizeof( DataType::TexturedVertex ) * FaceVerticesCount>( m_VisibleFacesCount );
    const auto indicesDataSize  = ScaleToSecond<1, sizeof( IndexBufferType )>( m_IndexBufferSize = ScaleToSecond<1, FaceIndicesCount>( m_VisibleFacesCount ) );
    auto&      api              = MainApplication::GetInstance( ).GetVulkanAPI( );

    if ( m_BufferAllocation.targetChunk == nullptr )
        m_BufferAllocation = ChunkSolidBuffer::GetInstance( ).CreateBuffer( verticesDataSize, indicesDataSize );
    else
        m_BufferAllocation = ChunkSolidBuffer::GetInstance( ).AlterBuffer( m_BufferAllocation, verticesDataSize, indicesDataSize );

    const uint32_t indexOffset = ScaleToSecond<sizeof( DataType::TexturedVertex ), 1, uint32_t>( m_BufferAllocation.region.vertexStartingOffset );

    std::unique_ptr<DataType::TexturedVertex[]> chunkVertices = std::make_unique<DataType::TexturedVertex[]>( ScaleToSecond<1, FaceVerticesCount>( m_VisibleFacesCount ) );
    std::unique_ptr<IndexBufferType[]>          chunkIndices  = std::make_unique<IndexBufferType[]>( m_IndexBufferSize );

    auto AddFace = [ indexOffset, faceAdded = 0, chunkVerticesPtr = chunkVertices.get( ), chunkIndicesPtr = chunkIndices.get( ) ]( const std::array<DataType::TexturedVertex, FaceVerticesCount>& vertexArray, const glm::vec3& offset, std::array<bool, 8> sideTransparency ) mutable {
        chunkVerticesPtr[ 0 ] = vertexArray[ 0 ];
        chunkVerticesPtr[ 0 ].pos += offset;
        chunkVerticesPtr[ 0 ].textureCoor_ColorIntensity.z *= sideTransparency[ 0 ] && sideTransparency[ 2 ] ? 0.2f : ( 3 - (int) sideTransparency[ 0 ] - (int) sideTransparency[ 1 ] - (int) sideTransparency[ 2 ] ) * 0.333333f;

        chunkVerticesPtr[ 1 ] = vertexArray[ 1 ];
        chunkVerticesPtr[ 1 ].pos += offset;
        chunkVerticesPtr[ 1 ].textureCoor_ColorIntensity.z *= sideTransparency[ 2 ] && sideTransparency[ 4 ] ? 0.2f : ( 3 - (int) sideTransparency[ 2 ] - (int) sideTransparency[ 3 ] - (int) sideTransparency[ 4 ] ) * 0.333333f;

        // TODO: front up dir seems to be not working
        chunkVerticesPtr[ 2 ] = vertexArray[ 2 ];
        chunkVerticesPtr[ 2 ].pos += offset;
        chunkVerticesPtr[ 2 ].textureCoor_ColorIntensity.z *= sideTransparency[ 4 ] && sideTransparency[ 6 ] ? 0.2f : ( 3 - (int) sideTransparency[ 4 ] - (int) sideTransparency[ 5 ] - (int) sideTransparency[ 6 ] ) * 0.333333f;

        chunkVerticesPtr[ 3 ] = vertexArray[ 3 ];
        chunkVerticesPtr[ 3 ].pos += offset;
        chunkVerticesPtr[ 3 ].textureCoor_ColorIntensity.z *= sideTransparency[ 6 ] && sideTransparency[ 0 ] ? 0.2f : ( 3 - (int) sideTransparency[ 6 ] - (int) sideTransparency[ 7 ] - (int) sideTransparency[ 0 ] ) * 0.333333f;

        chunkVerticesPtr += FaceVerticesCount;

        for ( int k = 0; k < FaceIndicesCount; ++k, ++chunkIndicesPtr )
        {
            *chunkIndicesPtr = blockIndices[ k ] + ScaleToSecond<1, FaceVerticesCount>( faceAdded ) + indexOffset;
        }

        ++faceAdded;
    };

    const auto& blockTextures = Minecraft::GetInstance( ).GetBlockTextures( );
    for ( int y = 0, blockIndex = 0; y < ChunkMaxHeight; ++y )
    {
        for ( int z = 0; z < SectionUnitLength; ++z )
        {
            for ( int x = 0; x < SectionUnitLength; ++x, ++blockIndex )
            {
                if ( const auto neighborTransparency = m_NeighborTransparency[ blockIndex ] )
                {
                    const auto& textures = blockTextures.GetTextureLocation( m_Blocks[ blockIndex ] );
                    const auto  offset   = glm::vec3( chunkX + x, y, chunkZ + z );

#define ToNotBoolArray( A, B, C, D, E, F, G, H )                                                       \
    {                                                                                                  \
        !bool( A ), !bool( B ), !bool( C ), !bool( D ), !bool( E ), !bool( F ), !bool( F ), !bool( H ) \
    }

                    if ( neighborTransparency & DirFrontBit )
                        AddFace( textures[ DirFront ], offset, ToNotBoolArray( neighborTransparency & DirFrontRightBit, neighborTransparency & DirFrontRightDownBit, neighborTransparency & DirFrontDownBit, neighborTransparency & DirFrontLeftDownBit, neighborTransparency & DirFrontLeftBit, neighborTransparency & DirFrontLeftUpBit, neighborTransparency & DirFrontUpBit, neighborTransparency & DirFrontRightUpBit ) );
                    if ( neighborTransparency & DirBackBit )
                        AddFace( textures[ DirBack ], offset, ToNotBoolArray( true, true, true, true, true, true, true, true ) );
                    if ( neighborTransparency & DirRightBit )
                        AddFace( textures[ DirRight ], offset, ToNotBoolArray( true, true, true, true, true, true, true, true ) );
                    if ( neighborTransparency & DirLeftBit )
                        AddFace( textures[ DirLeft ], offset, ToNotBoolArray( true, true, true, true, true, true, true, true ) );
                    if ( neighborTransparency & DirUpBit )
                        AddFace( textures[ DirUp ], offset, ToNotBoolArray( true, true, true, true, true, true, true, true ) );
                    if ( neighborTransparency & DirDownBit )
                        AddFace( textures[ DirDown ], offset, ToNotBoolArray( true, true, true, true, true, true, true, true ) );
                }

#undef ToBoolArray
            }
        }
    }

    ChunkSolidBuffer::GetInstance( ).CopyBuffer( m_BufferAllocation, chunkVertices.get( ), chunkIndices.get( ) );
}

bool
RenderableChunk::SetBlock( const BlockCoordinate& blockCoordinate, const Block& block )
{
    const auto& blockIndex = ScaleToSecond<1, SectionSurfaceSize>( GetMinecraftY( blockCoordinate ) ) + ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( blockCoordinate ) ) + GetMinecraftX( blockCoordinate );
    assert( blockIndex >= 0 && blockIndex < ChunkVolume );
    assert( std::all_of( m_NearChunks.begin( ), m_NearChunks.end( ), []( const auto& chunk ) { return chunk != nullptr; } ) );

    bool transparencyChanged = At( blockIndex ).Transparent( ) ^ block.Transparent( );
    auto successSetting      = Chunk::SetBlock( blockIndex, block );

    if ( !successSetting ) return false;
    if ( transparencyChanged && m_EmptySlot == 0 )
    {
        static constexpr auto dirUpFaceOffset    = SectionSurfaceSize;
        static constexpr auto dirDownFaceOffset  = -SectionSurfaceSize;
        static constexpr auto dirRightFaceOffset = SectionUnitLength;
        static constexpr auto dirLeftFaceOffset  = -SectionUnitLength;
        static constexpr auto dirFrontFaceOffset = 1;
        static constexpr auto dirBackFaceOffset  = -1;

        const auto originalFaceCount = std::popcount( m_NeighborTransparency[ blockIndex ] & DirFaceMask );
        UpdateNeighborAt( blockIndex );
        const auto newFaceCount = std::popcount( m_NeighborTransparency[ blockIndex ] & DirFaceMask );

        int         faceDiff     = 0;
        const auto* blockPtr     = m_Blocks.get( ) + blockIndex;
        auto* const blockFacePtr = m_NeighborTransparency + blockIndex;
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
        {
            if ( !blockPtr[ dirRightFaceOffset ].Transparent( ) )
            {
                blockFacePtr[ dirRightFaceOffset ] ^= DirLeftBit;
                ++faceDiff;
            }
        } else
        {

            if ( !m_NearChunks[ DirRight ]->At( blockIndex + dirRightChunkFaceOffset ).Transparent( ) )
            {
                m_NearChunks[ DirRight ]->m_NeighborTransparency[ blockIndex + dirRightChunkFaceOffset ] ^= DirLeftBit;
                m_NearChunks[ DirRight ]->m_VisibleFacesCount += block.Transparent( ) ? 1 : -1;
                m_NearChunks[ DirRight ]->SyncChunkFromDirection( this, DirLeft, true );
            }
        }

        if ( GetMinecraftZ( blockCoordinate ) != 0 )
        {
            if ( !blockPtr[ dirLeftFaceOffset ].Transparent( ) )
            {
                blockFacePtr[ dirLeftFaceOffset ] ^= DirRightBit;
                ++faceDiff;
            }
        } else
        {

            if ( !m_NearChunks[ DirLeft ]->At( blockIndex + dirLeftChunkFaceOffset ).Transparent( ) )
            {
                m_NearChunks[ DirLeft ]->m_NeighborTransparency[ blockIndex + dirLeftChunkFaceOffset ] ^= DirRightBit;
                m_NearChunks[ DirLeft ]->m_VisibleFacesCount += block.Transparent( ) ? 1 : -1;
                m_NearChunks[ DirLeft ]->SyncChunkFromDirection( this, DirRight, true );
            }
        }

        if ( GetMinecraftX( blockCoordinate ) != SectionUnitLength - 1 )
        {
            if ( !blockPtr[ dirFrontFaceOffset ].Transparent( ) )
            {
                blockFacePtr[ dirFrontFaceOffset ] ^= DirBackBit;
                ++faceDiff;
            }
        } else
        {

            if ( !m_NearChunks[ DirFront ]->At( blockIndex + dirFrontChunkFaceOffset ).Transparent( ) )
            {
                m_NearChunks[ DirFront ]->m_NeighborTransparency[ blockIndex + dirFrontChunkFaceOffset ] ^= DirBackBit;
                m_NearChunks[ DirFront ]->m_VisibleFacesCount += block.Transparent( ) ? 1 : -1;
                m_NearChunks[ DirFront ]->SyncChunkFromDirection( this, DirBack, true );
            }
        }

        if ( GetMinecraftX( blockCoordinate ) != 0 )
        {
            if ( !blockPtr[ dirBackFaceOffset ].Transparent( ) )
            {
                blockFacePtr[ dirBackFaceOffset ] ^= DirFrontBit;
                ++faceDiff;
            }
        } else
        {

            if ( !m_NearChunks[ DirBack ]->At( blockIndex + dirBackChunkFaceOffset ).Transparent( ) )
            {
                m_NearChunks[ DirBack ]->m_NeighborTransparency[ blockIndex + dirBackChunkFaceOffset ] ^= DirFrontBit;
                m_NearChunks[ DirBack ]->m_VisibleFacesCount += block.Transparent( ) ? 1 : -1;
                m_NearChunks[ DirBack ]->SyncChunkFromDirection( this, DirFront, true );
            }
        }

        if ( block.Transparent( ) )
        {
            m_VisibleFacesCount += faceDiff - originalFaceCount;
        } else
        {
            m_VisibleFacesCount += newFaceCount - faceDiff;
        }

#ifndef NDEBUG

        int checkingFaceCount = 0;
        for ( int i = 0; i < ChunkVolume; ++i )
            checkingFaceCount += std::popcount( m_NeighborTransparency[ i ] & DirFaceMask );
        assert( m_VisibleFacesCount == checkingFaceCount );
#endif
    }

    return true;
}

bool
RenderableChunk::SyncChunkFromDirection( RenderableChunk* other, Direction fromDir, bool changes )
{
    std::lock_guard<std::recursive_mutex> lock( m_SyncMutex );
    if ( m_EmptySlot == 0 )
    {
        if ( !( m_NearChunks[ fromDir ] = other ) )
        {
            // Logger::getInstance( ).LogLine( Logger::LogType::eVerbose, GetCoordinate( ), "are now incomplete chunks" );
            m_EmptySlot |= 1 << fromDir;

        } else if ( changes )   // not necessarily, but performance tho
        {
            GenerateRenderBuffer( );
        }
    } else
    {
        if ( ( m_NearChunks[ fromDir ] = other ) )
            m_EmptySlot &= ~( 1 << fromDir );
        else
            m_EmptySlot |= ( 1 << fromDir );

        if ( m_EmptySlot == 0 )
        {
            // Chunk surrounding completed
            ResetRenderBuffer( );
            RegenerateVisibleFaces( );
            return true;
        }
    }

    return false;
}