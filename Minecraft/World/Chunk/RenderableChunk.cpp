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

namespace
{
inline auto&
BlockAt( const std::pair<RenderableChunk*, uint32_t>& chunkIndexPair, uint32_t indexOffset )
{
    return chunkIndexPair.first->At( chunkIndexPair.second + indexOffset );
}
}   // namespace

void
RenderableChunk::UpdateNeighborAt( uint32_t index )
{
    assert( m_EmptySlot == 0 );
    assert( std::all_of( m_NearChunks.begin( ), m_NearChunks.end( ), []( const auto& chunk ) { return chunk != nullptr; } ) );
    assert( index < ChunkVolume );
    assert( m_NeighborTransparency != nullptr );

    // TODO: use zero to reset
    auto& blockNeighborTransparency = m_NeighborTransparency[ index ] = 0;

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

#define CHUNK_DIR( dir ) \
    if ( BlockAt( m_HorizontalIndexChunkAfterPointMoved[ EW##dir ], 0 ).Transparent( ) ) blockNeighborTransparency |= dir##Bit;

#define CHUNK_DIR_OFFSET( dir, offset, DirKeyword ) \
    if ( BlockAt( m_HorizontalIndexChunkAfterPointMoved[ EW##dir ], offset ).Transparent( ) ) blockNeighborTransparency |= dir##DirKeyword##Bit;

#define SAVE_SIDEWAYS_CHUNK_INDEX_OFFSET( dir, chunkPtrDir, offset ) m_HorizontalIndexChunkAfterPointMoved[ ( dir ) ] = { m_NearChunks[ ( chunkPtrDir ) ], \
                                                                                                                          index + ( offset ) }
#define SAVE_CHUNK_INDEX_OFFSET( dir, offset )      m_HorizontalIndexChunkAfterPointMoved[ ( dir ) ] = { m_NearChunks[ ( dir ) ], index + ( offset ) }
#define SAVE_THIS_CHUNK_INDEX_OFFSET( dir, offset ) m_HorizontalIndexChunkAfterPointMoved[ ( dir ) ] = { this, index + ( offset ) }

#define DIAGONAL_CHECK( X, Y )                                                                                       \
    if ( blockAtMost##X ) [[unlikely]]                                                                               \
    {                                                                                                                \
        if ( blockAtMost##Y ) [[unlikely]]                                                                           \
        {                                                                                                            \
            SAVE_CHUNK_INDEX_OFFSET( EWDir##Y##X, dir##Y##ChunkFaceOffset + dir##X##ChunkFaceOffset );               \
        } else                                                                                                       \
        {                                                                                                            \
            SAVE_SIDEWAYS_CHUNK_INDEX_OFFSET( EWDir##Y##X, EWDir##X, dir##Y##FaceOffset + dir##X##ChunkFaceOffset ); \
        }                                                                                                            \
    } else                                                                                                           \
    {                                                                                                                \
        if ( blockAtMost##Y ) [[unlikely]]                                                                           \
        {                                                                                                            \
            SAVE_SIDEWAYS_CHUNK_INDEX_OFFSET( EWDir##Y##X, EWDir##Y, dir##Y##ChunkFaceOffset + dir##X##FaceOffset ); \
        } else                                                                                                       \
        {                                                                                                            \
            SAVE_THIS_CHUNK_INDEX_OFFSET( EWDir##Y##X, dir##Y##FaceOffset + dir##X##FaceOffset );                    \
        }                                                                                                            \
    }                                                                                                                \
    if ( BlockAt( m_HorizontalIndexChunkAfterPointMoved[ EWDir##Y##X ], 0 ).Transparent( ) )                         \
    {                                                                                                                \
        blockNeighborTransparency |= Dir##Y##X##Bit;                                                                 \
    }

    /*********/
    /* Right */
    /*********/
    std::array<std::pair<RenderableChunk*, uint32_t>, EightWayDirectionSize> m_HorizontalIndexChunkAfterPointMoved;
    if ( blockAtMostRight ) [[unlikely]]
        SAVE_CHUNK_INDEX_OFFSET( EWDirRight, dirRightChunkFaceOffset );
    else
        SAVE_THIS_CHUNK_INDEX_OFFSET( EWDirRight, dirRightFaceOffset );
    CHUNK_DIR( DirRight );

    /*********/
    /* Left */
    /*********/
    if ( blockAtMostLeft ) [[unlikely]]
        SAVE_CHUNK_INDEX_OFFSET( EWDirLeft, dirLeftChunkFaceOffset );
    else
        SAVE_THIS_CHUNK_INDEX_OFFSET( EWDirLeft, dirLeftFaceOffset );
    CHUNK_DIR( DirLeft );

    /*********/
    /* Front */
    /*********/
    if ( blockAtMostFront ) [[unlikely]]
        SAVE_CHUNK_INDEX_OFFSET( EWDirFront, dirFrontChunkFaceOffset );
    else
        SAVE_THIS_CHUNK_INDEX_OFFSET( EWDirFront, dirFrontFaceOffset );
    CHUNK_DIR( DirFront );

    /*********/
    /* Back */
    /*********/
    if ( blockAtMostBack ) [[unlikely]]
        SAVE_CHUNK_INDEX_OFFSET( EWDirBack, dirBackChunkFaceOffset );
    else
        SAVE_THIS_CHUNK_INDEX_OFFSET( EWDirBack, dirBackFaceOffset );
    CHUNK_DIR( DirBack );

    /***************/
    /* Front Right */
    /***************/
    DIAGONAL_CHECK( Right, Front );

    /**************/
    /* Front Left */
    /**************/
    DIAGONAL_CHECK( Left, Front );

    /***************/
    /* Back Right */
    /***************/
    DIAGONAL_CHECK( Right, Back );

    /*************/
    /* Back Left */
    /*************/
    DIAGONAL_CHECK( Left, Back );

    /***********/
    /* All top */
    /***********/
    if ( !blockOnTop )
    {
        CHUNK_DIR_OFFSET( DirFront, dirUpFaceOffset, Up );
        CHUNK_DIR_OFFSET( DirBack, dirUpFaceOffset, Up );
        CHUNK_DIR_OFFSET( DirRight, dirUpFaceOffset, Up );
        CHUNK_DIR_OFFSET( DirLeft, dirUpFaceOffset, Up );
        CHUNK_DIR_OFFSET( DirFrontRight, dirUpFaceOffset, Up );
        CHUNK_DIR_OFFSET( DirFrontLeft, dirUpFaceOffset, Up );
        CHUNK_DIR_OFFSET( DirBackRight, dirUpFaceOffset, Up );
        CHUNK_DIR_OFFSET( DirBackLeft, dirUpFaceOffset, Up );
    }

    /**************/
    /* All bottom */
    /**************/
    if ( !blockOnBottom )
    {
        CHUNK_DIR_OFFSET( DirFront, dirDownFaceOffset, Down );
        CHUNK_DIR_OFFSET( DirBack, dirDownFaceOffset, Down );
        CHUNK_DIR_OFFSET( DirRight, dirDownFaceOffset, Down );
        CHUNK_DIR_OFFSET( DirLeft, dirDownFaceOffset, Down );
        CHUNK_DIR_OFFSET( DirFrontRight, dirDownFaceOffset, Down );
        CHUNK_DIR_OFFSET( DirFrontLeft, dirDownFaceOffset, Down );
        CHUNK_DIR_OFFSET( DirBackRight, dirDownFaceOffset, Down );
        CHUNK_DIR_OFFSET( DirBackLeft, dirDownFaceOffset, Down );
    }

#undef SAVE_CHUNK_INDEX_OFFSET
#undef SAVE_THIS_CHUNK_INDEX_OFFSET
#undef SAVE_SIDEWAYS_CHUNK_INDEX_OFFSET
#undef DIAGONAL_CHECK
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
        static constexpr auto faceShaderMultiplier = 1.0f / 3;

        chunkVerticesPtr[ 0 ] = vertexArray[ 0 ];
        chunkVerticesPtr[ 0 ].pos += offset;
        chunkVerticesPtr[ 0 ].textureCoor_ColorIntensity.z *= sideTransparency[ 0 ] && sideTransparency[ 2 ] ? 0.2f : ( 2 - (int) sideTransparency[ 0 ] - (int) sideTransparency[ 1 ] - (int) sideTransparency[ 2 ] ) * faceShaderMultiplier;

        chunkVerticesPtr[ 1 ] = vertexArray[ 1 ];
        chunkVerticesPtr[ 1 ].pos += offset;
        chunkVerticesPtr[ 1 ].textureCoor_ColorIntensity.z *= sideTransparency[ 2 ] && sideTransparency[ 4 ] ? 0.2f : ( 2 - (int) sideTransparency[ 2 ] - (int) sideTransparency[ 3 ] - (int) sideTransparency[ 4 ] ) * faceShaderMultiplier;

        chunkVerticesPtr[ 2 ] = vertexArray[ 2 ];
        chunkVerticesPtr[ 2 ].pos += offset;
        chunkVerticesPtr[ 2 ].textureCoor_ColorIntensity.z *= sideTransparency[ 4 ] && sideTransparency[ 6 ] ? 0.2f : ( 2 - (int) sideTransparency[ 4 ] - (int) sideTransparency[ 5 ] - (int) sideTransparency[ 6 ] ) * faceShaderMultiplier;

        chunkVerticesPtr[ 3 ] = vertexArray[ 3 ];
        chunkVerticesPtr[ 3 ].pos += offset;
        chunkVerticesPtr[ 3 ].textureCoor_ColorIntensity.z *= sideTransparency[ 6 ] && sideTransparency[ 0 ] ? 0.2f : ( 2 - (int) sideTransparency[ 6 ] - (int) sideTransparency[ 7 ] - (int) sideTransparency[ 0 ] ) * faceShaderMultiplier;

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

                    // for ambient occlusion
#define ToNotBoolArray( A, B, C, D, E, F, G, H )                                                       \
    {                                                                                                  \
        !bool( A ), !bool( B ), !bool( C ), !bool( D ), !bool( E ), !bool( F ), !bool( G ), !bool( H ) \
    }

                    if ( neighborTransparency & DirFrontBit )
                        AddFace( textures[ DirFront ], offset, ToNotBoolArray( neighborTransparency & DirFrontRightBit, neighborTransparency & DirFrontRightDownBit, neighborTransparency & DirFrontDownBit, neighborTransparency & DirFrontLeftDownBit, neighborTransparency & DirFrontLeftBit, neighborTransparency & DirFrontLeftUpBit, neighborTransparency & DirFrontUpBit, neighborTransparency & DirFrontRightUpBit ) );
                    if ( neighborTransparency & DirBackBit )
                        AddFace( textures[ DirBack ], offset, ToNotBoolArray( neighborTransparency & DirBackLeftBit, neighborTransparency & DirBackLeftDownBit, neighborTransparency & DirBackDownBit, neighborTransparency & DirBackRightDownBit, neighborTransparency & DirBackRightBit, neighborTransparency & DirBackRightUpBit, neighborTransparency & DirBackUpBit, neighborTransparency & DirBackLeftUpBit ) );
                    if ( neighborTransparency & DirRightBit )
                        AddFace( textures[ DirRight ], offset, ToNotBoolArray( neighborTransparency & DirBackRightBit, neighborTransparency & DirBackRightDownBit, neighborTransparency & DirRightDownBit, neighborTransparency & DirFrontRightDownBit, neighborTransparency & DirFrontRightBit, neighborTransparency & DirFrontRightUpBit, neighborTransparency & DirRightUpBit, neighborTransparency & DirBackRightUpBit ) );
                    if ( neighborTransparency & DirLeftBit )
                        AddFace( textures[ DirLeft ], offset, ToNotBoolArray( neighborTransparency & DirFrontLeftBit, neighborTransparency & DirFrontLeftDownBit, neighborTransparency & DirLeftDownBit, neighborTransparency & DirBackLeftDownBit, neighborTransparency & DirBackLeftBit, neighborTransparency & DirBackLeftUpBit, neighborTransparency & DirLeftUpBit, neighborTransparency & DirFrontLeftUpBit ) );
                    if ( neighborTransparency & DirUpBit )
                        AddFace( textures[ DirUp ], offset, ToNotBoolArray( neighborTransparency & DirLeftUpBit, neighborTransparency & DirBackLeftUpBit, neighborTransparency & DirBackUpBit, neighborTransparency & DirBackRightUpBit, neighborTransparency & DirRightUpBit, neighborTransparency & DirFrontRightUpBit, neighborTransparency & DirFrontUpBit, neighborTransparency & DirFrontLeftUpBit ) );
                    if ( neighborTransparency & DirDownBit )
                        AddFace( textures[ DirDown ], offset, ToNotBoolArray( neighborTransparency & DirLeftDownBit, neighborTransparency & DirFrontLeftDownBit, neighborTransparency & DirFrontDownBit, neighborTransparency & DirFrontRightDownBit, neighborTransparency & DirRightDownBit, neighborTransparency & DirBackRightDownBit, neighborTransparency & DirBackDownBit, neighborTransparency & DirBackLeftDownBit ) );
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

        // TODO: modify nearby blocks including diagonal
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
                m_NearChunks[ DirRight ]->SyncChunkFromDirection( this, EWDirLeft, true );
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
                m_NearChunks[ DirLeft ]->SyncChunkFromDirection( this, EWDirRight, true );
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
                m_NearChunks[ DirFront ]->SyncChunkFromDirection( this, EWDirBack, true );
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
                m_NearChunks[ DirBack ]->SyncChunkFromDirection( this, EWDirFront, true );
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
RenderableChunk::SyncChunkFromDirection( RenderableChunk* other, EightWayDirection fromDir, bool changes )
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