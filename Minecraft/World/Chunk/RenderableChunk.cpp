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
    m_BlockFaces = new uint8_t[ ChunkVolume ];
}

void
RenderableChunk::RegenerateVisibleFaces( )
{
    std::lock_guard<std::recursive_mutex> lock( m_SyncMutex );

    for ( int i = 0; i < ChunkVolume; ++i )
        RegenerateVisibleFacesAt( i );

    m_VisibleFacesCount = 0;
    for ( int i = 0; i < ChunkVolume; ++i )
        m_VisibleFacesCount += std::popcount( m_BlockFaces[ i ] );
}

void
RenderableChunk::RegenerateVisibleFacesAt( const uint32_t index )
{
    assert( m_EmptySlot == 0 );
    assert( std::all_of( m_NearChunks.begin( ), m_NearChunks.end( ), []( const auto& chunk ) { return chunk != nullptr; } ) );
    assert( index >= 0 && index < ChunkVolume );
    assert( m_BlockFaces != nullptr );

    auto& blockFace = m_BlockFaces[ index ] = 0;
    if ( At( index ).Transparent( ) ) return;

    if ( ( index >> SectionSurfaceSizeBinaryOffset ) == ChunkMaxHeight - 1 || At( index + dirUpFaceOffset ).Transparent( ) ) [[unlikely]]
        blockFace = DirUpBit;
    if ( ( index >> SectionSurfaceSizeBinaryOffset ) == 0 || At( index + dirDownFaceOffset ).Transparent( ) ) [[unlikely]]
        blockFace |= DirDownBit;

    if ( ( index % SectionSurfaceSize ) >= SectionSurfaceSize - SectionUnitLength ) [[unlikely]]
    {
        if ( m_NearChunks[ DirRight ]->At( index + dirRightChunkFaceOffset ).Transparent( ) )
        {
            blockFace |= DirRightBit;
        }

    } else if ( At( index + dirRightFaceOffset ).Transparent( ) )
    {
        blockFace |= DirRightBit;
    }

    if ( ( index % SectionSurfaceSize ) < SectionUnitLength ) [[unlikely]]
    {
        if ( m_NearChunks[ DirLeft ]->At( index + dirLeftChunkFaceOffset ).Transparent( ) )
        {
            blockFace |= DirLeftBit;
        }

    } else if ( At( index + dirLeftFaceOffset ).Transparent( ) )
    {
        blockFace |= DirLeftBit;
    }

    if ( ( index % SectionUnitLength ) == SectionUnitLength - 1 ) [[unlikely]]
    {
        if ( m_NearChunks[ DirFront ]->At( index + dirFrontChunkFaceOffset ).Transparent( ) )
        {
            blockFace |= DirFrontBit;
        }

    } else if ( At( index + dirFrontFaceOffset ).Transparent( ) )
    {
        blockFace |= DirFrontBit;
    }

    if ( ( index % SectionUnitLength ) == 0 ) [[unlikely]]
    {
        if ( m_NearChunks[ DirBack ]->At( index + dirBackChunkFaceOffset ).Transparent( ) )
        {
            blockFace |= DirBackBit;
        }

    } else if ( At( index + dirBackFaceOffset ).Transparent( ) )
    {
        blockFace |= DirBackBit;
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

    auto AddFace = [ indexOffset, faceAdded = 0, chunkVerticesPtr = chunkVertices.get( ), chunkIndicesPtr = chunkIndices.get( ) ]( const std::array<DataType::TexturedVertex, FaceVerticesCount>& vertexArray, const glm::vec3& offset ) mutable {
        for ( int k = 0; k < FaceVerticesCount; ++k, ++chunkVerticesPtr )
        {
            *chunkVerticesPtr = vertexArray[ k ];
            chunkVerticesPtr->pos += offset;
        }

        for ( int k = 0; k < FaceIndicesCount; ++k, ++chunkIndicesPtr )
        {
            *chunkIndicesPtr = blockIndices[ k ] + faceAdded * FaceVerticesCount + indexOffset;
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
                if ( const auto visibleFace = m_BlockFaces[ blockIndex ] )
                {
                    const auto& textures = blockTextures.GetTextureLocation( m_Blocks[ blockIndex ] );
                    const auto  offset   = glm::vec3( chunkX + x, y, chunkZ + z );
                    if ( visibleFace & DirFrontBit )
                        AddFace( textures[ DirFront ], offset );
                    if ( visibleFace & DirBackBit )
                        AddFace( textures[ DirBack ], offset );
                    if ( visibleFace & DirRightBit )
                        AddFace( textures[ DirRight ], offset );
                    if ( visibleFace & DirLeftBit )
                        AddFace( textures[ DirLeft ], offset );
                    if ( visibleFace & DirUpBit )
                        AddFace( textures[ DirUp ], offset );
                    if ( visibleFace & DirDownBit )
                        AddFace( textures[ DirDown ], offset );
                }
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

        const auto originalFaceCount = std::popcount( m_BlockFaces[ blockIndex ] );
        RegenerateVisibleFacesAt( blockIndex );
        const auto newFaceCount = std::popcount( m_BlockFaces[ blockIndex ] );

        int         faceDiff     = 0;
        const auto* blockPtr     = m_Blocks.get( ) + blockIndex;
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
                m_NearChunks[ DirRight ]->m_BlockFaces[ blockIndex + dirRightChunkFaceOffset ] ^= DirLeftBit;
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
                m_NearChunks[ DirLeft ]->m_BlockFaces[ blockIndex + dirLeftChunkFaceOffset ] ^= DirRightBit;
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
                m_NearChunks[ DirFront ]->m_BlockFaces[ blockIndex + dirFrontChunkFaceOffset ] ^= DirBackBit;
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
                m_NearChunks[ DirBack ]->m_BlockFaces[ blockIndex + dirBackChunkFaceOffset ] ^= DirFrontBit;
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
            checkingFaceCount += std::popcount( m_BlockFaces[ i ] );
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