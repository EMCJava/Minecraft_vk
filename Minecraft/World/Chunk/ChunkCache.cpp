//
// Created by loys on 5/24/22.
//

#include <Minecraft/Application/MainApplication.hpp>
#include <Minecraft/util/MinecraftConstants.hpp>

#include <Graphic/Vulkan/VulkanAPI.hpp>

#include <Utility/Logger.hpp>

#include "ChunkCache.hpp"

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
ChunkCache::RegenerateChunk( )
{
    DeleteCache( );
    m_BlockFaces = new uint8_t[ ChunkVolume ];

    Chunk::RegenerateChunk( );
}

void
ChunkCache::RegenerateVisibleFaces( )
{
    for ( int i = 0; i < ChunkVolume; ++i )
        RegenerateVisibleFacesAt( i );

    m_VisibleFacesCount = 0;
    for ( int i = 0; i < ChunkVolume; ++i )
        m_VisibleFacesCount += std::popcount( m_BlockFaces[ i ] );
}

void
ChunkCache::RegenerateVisibleFacesAt( uint32_t index )
{
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


void
ChunkCache::GenerateRenderBuffer( )
{
    if ( m_VisibleFacesCount == 0 ) return;

    // Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Generating chunk:", chunk.GetCoordinate( ) );

    auto [ chunkX, chunkZ, chunkY ] = GetCoordinate( );

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
ChunkCache::SetBlock( const BlockCoordinate& blockCoordinate, const Block& block )
{
    const auto& blockIndex = ScaleToSecond<1, SectionSurfaceSize>( GetMinecraftY( blockCoordinate ) ) + ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( blockCoordinate ) ) + GetMinecraftX( blockCoordinate );
    assert( blockIndex >= 0 && blockIndex < ChunkVolume );

    bool transparencyChanged = m_Blocks[ blockIndex ].Transparent( ) ^ block.Transparent( );
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
        {
            if ( !blockPtr[ dirRightFaceOffset ].Transparent( ) )
            {
                blockFacePtr[ dirRightFaceOffset ] ^= DirLeftBit;
                ++faceDiff;
            }
        } else
        {

            if ( !m_NearChunks[ DirRight ]->m_Blocks[ blockIndex + dirRightChunkFaceOffset ].Transparent( ) )
            {
                m_NearChunks[ DirRight ]->m_BlockFaces[ blockIndex + dirRightChunkFaceOffset ] ^= DirLeftBit;
                m_NearChunks[ DirRight ]->m_VisibleFacesCount += block.Transparent( ) ? 1 : -1;
                m_NearChunks[ DirRight ]->SyncChunkFromDirection( this, DirLeft, true );
                Logger::getInstance().LogLine("DirRight");
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

            if ( !m_NearChunks[ DirLeft ]->m_Blocks[ blockIndex + dirLeftChunkFaceOffset ].Transparent( ) )
            {
                m_NearChunks[ DirLeft ]->m_BlockFaces[ blockIndex + dirLeftChunkFaceOffset ] ^= DirRightBit;
                m_NearChunks[ DirLeft ]->m_VisibleFacesCount += block.Transparent( ) ? 1 : -1;
                m_NearChunks[ DirLeft ]->SyncChunkFromDirection( this, DirRight, true );
                Logger::getInstance().LogLine("DirLeft");
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

            if ( !m_NearChunks[ DirFront ]->m_Blocks[ blockIndex + dirFrontChunkFaceOffset ].Transparent( ) )
            {
                m_NearChunks[ DirFront ]->m_BlockFaces[ blockIndex + dirFrontChunkFaceOffset ] ^= DirBackBit;
                m_NearChunks[ DirFront ]->m_VisibleFacesCount += block.Transparent( ) ? 1 : -1;
                m_NearChunks[ DirFront ]->SyncChunkFromDirection( this, DirBack, true );
                Logger::getInstance().LogLine("DirFront");
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

            if ( !m_NearChunks[ DirBack ]->m_Blocks[ blockIndex + dirBackChunkFaceOffset ].Transparent( ) )
            {
                m_NearChunks[ DirBack ]->m_BlockFaces[ blockIndex + dirBackChunkFaceOffset ] ^= DirFrontBit;
                m_NearChunks[ DirBack ]->m_VisibleFacesCount += block.Transparent( ) ? 1 : -1;
                m_NearChunks[ DirBack ]->SyncChunkFromDirection( this, DirFront, true );
                Logger::getInstance().LogLine("DirBack");
            }
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

bool
ChunkCache::SyncChunkFromDirection( ChunkCache* other, Direction fromDir, bool changes )
{
    if ( m_EmptySlot == 0 )
    {
        if ( m_NearChunks[ fromDir ] == nullptr )
        {
            m_NearChunks[ fromDir ] = other;
        } else if ( changes )
        {
            GenerateRenderBuffer( );
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
}
