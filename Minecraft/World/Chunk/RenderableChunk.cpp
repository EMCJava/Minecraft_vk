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
    m_VertexMetaData       = new CubeVertexMetaData[ ChunkVolume ];
}

void
RenderableChunk::RegenerateVisibleFaces( )
{
    std::lock_guard<std::recursive_mutex> lock( m_SyncMutex );

    for ( int i = 0; i < ChunkVolume; ++i )
        UpdateNeighborAt( i );

    m_VisibleFacesCount = 0;
    for ( int i = 0; i < ChunkVolume; ++i )
    {
        // only count faces directed covered
        m_VisibleFacesCount += std::popcount( m_NeighborTransparency[ i ] & DirFaceMask );
        UpdateMetaDataAt( i );
    }
}

namespace
{
inline auto&
BlockAt( const std::pair<RenderableChunk*, uint32_t>& chunkIndexPair, uint32_t indexOffset )
{
    return chunkIndexPair.first->At( chunkIndexPair.second + indexOffset );
}
}   // namespace

std::array<std::pair<RenderableChunk*, uint32_t>, EightWayDirectionSize>
RenderableChunk::GetHorizontalChunkAfterPointMoved( uint32_t index )
{

#define CHUNK_DIR( dir ) \
    if ( BlockAt( result[ EW##dir ], 0 ).Transparent( ) ) blockNeighborTransparency |= dir##Bit;

#define CHUNK_DIR_OFFSET( dir, offset, DirKeyword ) \
    if ( BlockAt( result[ EW##dir ], offset ).Transparent( ) ) blockNeighborTransparency |= dir##DirKeyword##Bit;

#define SAVE_SIDEWAYS_CHUNK_INDEX_OFFSET( dir, chunkPtrDir, offset ) result[ ( dir ) ] = { m_NearChunks[ ( chunkPtrDir ) ], \
                                                                                           index + ( offset ) }
#define SAVE_CHUNK_INDEX_OFFSET( dir, offset )      result[ ( dir ) ] = { m_NearChunks[ ( dir ) ], index + ( offset ) }
#define SAVE_THIS_CHUNK_INDEX_OFFSET( dir, offset ) result[ ( dir ) ] = { this, index + ( offset ) }

#define ADD_DIAGONAL( X, Y )                                                                                         \
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
    }

    const bool blockAtMostRight = ( index % SectionSurfaceSize ) >= SectionSurfaceSize - SectionUnitLength;
    const bool blockAtMostLeft  = ( index % SectionSurfaceSize ) < SectionUnitLength;
    const bool blockAtMostFront = ( index % SectionUnitLength ) == SectionUnitLength - 1;
    const bool blockAtMostBack  = ( index % SectionUnitLength ) == 0;

    std::array<std::pair<RenderableChunk*, uint32_t>, EightWayDirectionSize> result;

    /*********/
    /* Right */
    /*********/
    if ( blockAtMostRight ) [[unlikely]]
        SAVE_CHUNK_INDEX_OFFSET( EWDirRight, dirRightChunkFaceOffset );
    else
        SAVE_THIS_CHUNK_INDEX_OFFSET( EWDirRight, dirRightFaceOffset );

    /*********/
    /* Left */
    /*********/
    if ( blockAtMostLeft ) [[unlikely]]
        SAVE_CHUNK_INDEX_OFFSET( EWDirLeft, dirLeftChunkFaceOffset );
    else
        SAVE_THIS_CHUNK_INDEX_OFFSET( EWDirLeft, dirLeftFaceOffset );

    /*********/
    /* Front */
    /*********/
    if ( blockAtMostFront ) [[unlikely]]
        SAVE_CHUNK_INDEX_OFFSET( EWDirFront, dirFrontChunkFaceOffset );
    else
        SAVE_THIS_CHUNK_INDEX_OFFSET( EWDirFront, dirFrontFaceOffset );

    /*********/
    /* Back */
    /*********/
    if ( blockAtMostBack ) [[unlikely]]
        SAVE_CHUNK_INDEX_OFFSET( EWDirBack, dirBackChunkFaceOffset );
    else
        SAVE_THIS_CHUNK_INDEX_OFFSET( EWDirBack, dirBackFaceOffset );

    /***************/
    /* Front Right */
    /***************/
    ADD_DIAGONAL( Right, Front );

    /**************/
    /* Front Left */
    /**************/
    ADD_DIAGONAL( Left, Front );

    /***************/
    /* Back Right */
    /***************/
    ADD_DIAGONAL( Right, Back );

    /*************/
    /* Back Left */
    /*************/
    ADD_DIAGONAL( Left, Back );

#undef CHUNK_DIR
#undef CHUNK_DIR_OFFSET
#undef SAVE_CHUNK_INDEX_OFFSET
#undef SAVE_THIS_CHUNK_INDEX_OFFSET
#undef SAVE_SIDEWAYS_CHUNK_INDEX_OFFSET
#undef ADD_DIAGONAL

    return result;
}

void
RenderableChunk::UpdateNeighborAt( uint32_t index )
{
    assert( m_EmptySlot == 0 );
    assert( std::all_of( m_NearChunks.begin( ), m_NearChunks.end( ), []( const auto& chunk ) { return chunk != nullptr; } ) );
    assert( index < ChunkVolume );
    assert( m_NeighborTransparency != nullptr );

    auto& blockNeighborTransparency = m_NeighborTransparency[ index ] = 0;

    // not matter at this point
    if ( At( index ).Transparent( ) ) return;

    const bool blockOnTop    = indexToHeight( index ) == ChunkMaxHeight - 1;
    const bool blockOnBottom = indexToHeight( index ) == 0;

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

    const auto horizontalIndexChunkAfterPointMoved = GetHorizontalChunkAfterPointMoved( index );

#define CHUNK_DIR( dir ) \
    if ( BlockAt( horizontalIndexChunkAfterPointMoved[ EW##dir ], 0 ).Transparent( ) ) blockNeighborTransparency |= dir##Bit;
#define CHUNK_DIR_OFFSET( dir, DirKeyword, offset ) \
    if ( BlockAt( horizontalIndexChunkAfterPointMoved[ EW##dir ], offset ).Transparent( ) ) blockNeighborTransparency |= dir##DirKeyword##Bit;
#define DIAGONAL_CHECK( X, Y )                                                             \
    if ( BlockAt( horizontalIndexChunkAfterPointMoved[ EWDir##Y##X ], 0 ).Transparent( ) ) \
    {                                                                                      \
        blockNeighborTransparency |= Dir##Y##X##Bit;                                       \
    }

    CHUNK_DIR( DirRight );
    CHUNK_DIR( DirLeft );
    CHUNK_DIR( DirFront );
    CHUNK_DIR( DirBack );

    DIAGONAL_CHECK( Right, Front );
    DIAGONAL_CHECK( Left, Front );
    DIAGONAL_CHECK( Right, Back );
    DIAGONAL_CHECK( Left, Back );

    /***********/
    /* All top */
    /***********/
    if ( !blockOnTop ) [[likely]]
    {
        CHUNK_DIR_OFFSET( DirFront, Up, dirUpFaceOffset );
        CHUNK_DIR_OFFSET( DirBack, Up, dirUpFaceOffset );
        CHUNK_DIR_OFFSET( DirRight, Up, dirUpFaceOffset );
        CHUNK_DIR_OFFSET( DirLeft, Up, dirUpFaceOffset );
        CHUNK_DIR_OFFSET( DirFrontRight, Up, dirUpFaceOffset );
        CHUNK_DIR_OFFSET( DirFrontLeft, Up, dirUpFaceOffset );
        CHUNK_DIR_OFFSET( DirBackRight, Up, dirUpFaceOffset );
        CHUNK_DIR_OFFSET( DirBackLeft, Up, dirUpFaceOffset );
    }

    /**************/
    /* All bottom */
    /**************/
    if ( !blockOnBottom ) [[likely]]
    {
        CHUNK_DIR_OFFSET( DirFront, Down, dirDownFaceOffset );
        CHUNK_DIR_OFFSET( DirBack, Down, dirDownFaceOffset );
        CHUNK_DIR_OFFSET( DirRight, Down, dirDownFaceOffset );
        CHUNK_DIR_OFFSET( DirLeft, Down, dirDownFaceOffset );
        CHUNK_DIR_OFFSET( DirFrontRight, Down, dirDownFaceOffset );
        CHUNK_DIR_OFFSET( DirFrontLeft, Down, dirDownFaceOffset );
        CHUNK_DIR_OFFSET( DirBackRight, Down, dirDownFaceOffset );
        CHUNK_DIR_OFFSET( DirBackLeft, Down, dirDownFaceOffset );
    }

#undef CHUNK_DIR
#undef CHUNK_DIR_OFFSET
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

    static const std::array<IndexBufferType, FaceIndicesCount> blockIndices        = { 0, 1, 2, 2, 3, 0 };
    static const std::array<IndexBufferType, FaceIndicesCount> blockIndicesFlipped = { 0, 1, 3, 1, 2, 3 };

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

    auto AddFace = [ indexOffset, faceAdded = 0, chunkVerticesPtr = chunkVertices.get( ), chunkIndicesPtr = chunkIndices.get( ) ]( const std::array<DataType::TexturedVertex, FaceVerticesCount>& vertexArray, const glm::vec3& offset, const FaceVertexAmbientOcclusionData& faceAmbientOcclusionStrengths ) mutable {
        static constexpr auto faceShaderMultiplier = 1.0f / 3;

        for ( int i = 0; i < FaceVerticesCount; ++i )
        {
            chunkVerticesPtr[ i ] = vertexArray[ i ];
            chunkVerticesPtr[ i ].pos += offset;
            chunkVerticesPtr[ i ].textureCoor_ColorIntensity.z *= 0.2f + faceAmbientOcclusionStrengths.data[ i ] * faceShaderMultiplier;
        }

        chunkVerticesPtr += FaceVerticesCount;

        if ( faceAmbientOcclusionStrengths.data[ 3 ] + faceAmbientOcclusionStrengths.data[ 1 ] > faceAmbientOcclusionStrengths.data[ 0 ] + faceAmbientOcclusionStrengths.data[ 2 ] )
        {
            for ( int k = 0; k < FaceIndicesCount; ++k, ++chunkIndicesPtr )
            {
                *chunkIndicesPtr = blockIndicesFlipped[ k ] + ScaleToSecond<1, FaceVerticesCount>( faceAdded ) + indexOffset;
            }
        } else
        {
            for ( int k = 0; k < FaceIndicesCount; ++k, ++chunkIndicesPtr )
            {
                *chunkIndicesPtr = blockIndices[ k ] + ScaleToSecond<1, FaceVerticesCount>( faceAdded ) + indexOffset;
            }
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

                    if ( neighborTransparency & DirFrontBit )
                        AddFace( textures[ DirFront ], offset, m_VertexMetaData[ blockIndex ].faceVertexMetaData[ DirFront ].ambientOcclusionData );
                    if ( neighborTransparency & DirBackBit )
                        AddFace( textures[ DirBack ], offset, m_VertexMetaData[ blockIndex ].faceVertexMetaData[ DirBack ].ambientOcclusionData );
                    if ( neighborTransparency & DirRightBit )
                        AddFace( textures[ DirRight ], offset, m_VertexMetaData[ blockIndex ].faceVertexMetaData[ DirRight ].ambientOcclusionData );
                    if ( neighborTransparency & DirLeftBit )
                        AddFace( textures[ DirLeft ], offset, m_VertexMetaData[ blockIndex ].faceVertexMetaData[ DirLeft ].ambientOcclusionData );
                    if ( neighborTransparency & DirUpBit )
                        AddFace( textures[ DirUp ], offset, m_VertexMetaData[ blockIndex ].faceVertexMetaData[ DirUp ].ambientOcclusionData );
                    if ( neighborTransparency & DirDownBit )
                        AddFace( textures[ DirDown ], offset, m_VertexMetaData[ blockIndex ].faceVertexMetaData[ DirDown ].ambientOcclusionData );
                }
            }
        }
    }

    GenerateGreedyMesh( );

    ChunkSolidBuffer::GetInstance( ).CopyBuffer( m_BufferAllocation, chunkVertices.get( ), chunkIndices.get( ) );
}

bool
RenderableChunk::SetBlock( const BlockCoordinate& blockCoordinate, const Block& block )
{
    const auto& blockIndex = ScaleToSecond<1, SectionSurfaceSize>( GetMinecraftY( blockCoordinate ) ) + ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( blockCoordinate ) ) + GetMinecraftX( blockCoordinate );
    assert( blockIndex >= 0 && blockIndex < ChunkVolume );
    assert( std::all_of( m_NearChunks.begin( ), m_NearChunks.end( ), []( const auto& chunk ) { return chunk != nullptr; } ) );

    const auto isBlockTransparent  = block.Transparent( );
    const auto transparencyChanged = At( blockIndex ).Transparent( ) ^ isBlockTransparent;
    const auto successSetting      = Chunk::SetBlock( blockIndex, block );

    if ( !successSetting ) return false;
    if ( transparencyChanged && m_EmptySlot == 0 )
    {
        const auto faceCountDiff = isBlockTransparent ? 1 : -1;

        const bool blockOnTop    = indexToHeight( blockIndex ) == ChunkMaxHeight - 1;
        const bool blockOnBottom = indexToHeight( blockIndex ) == 0;

        const int originalFaceCount = std::popcount( m_NeighborTransparency[ blockIndex ] & DirFaceMask );
        UpdateNeighborAt( blockIndex );
        const int newFaceCount = std::popcount( m_NeighborTransparency[ blockIndex ] & DirFaceMask );

        m_VisibleFacesCount += newFaceCount - originalFaceCount;

        const auto* blockPtr = m_Blocks.get( ) + blockIndex;
        if ( !blockOnTop )
            if ( !blockPtr[ dirUpFaceOffset ].Transparent( ) )
            {
                m_NeighborTransparency[ blockIndex + dirUpFaceOffset ] ^= DirDownBit;
                UpdateAmbientOcclusionAt( blockIndex + dirUpFaceOffset );
                m_VisibleFacesCount += faceCountDiff;
            }

        if ( !blockOnBottom )
            if ( !blockPtr[ dirDownFaceOffset ].Transparent( ) )
            {
                m_NeighborTransparency[ blockIndex + dirDownFaceOffset ] ^= DirUpBit;
                UpdateAmbientOcclusionAt( blockIndex + dirDownFaceOffset );
                m_VisibleFacesCount += faceCountDiff;
            }

        const auto horizontalIndexChunkAfterPointMoved = GetHorizontalChunkAfterPointMoved( blockIndex );

#define UpdateAlongDir( dir )                                                                                               \
    {                                                                                                                       \
        constexpr auto oppositeDirectionBit     = DirectionBit( 1 << ( IntLog<(int) Dir##dir##Bit, 2>::value ^ 0b1 ) );     \
        constexpr auto oppositeDirectionDownBit = DirectionBit( 1 << ( IntLog<(int) Dir##dir##UpBit, 2>::value ^ 0b1 ) );   \
        constexpr auto oppositeDirectionUpBit   = DirectionBit( 1 << ( IntLog<(int) Dir##dir##DownBit, 2>::value ^ 0b1 ) ); \
        auto*          chunk                    = horizontalIndexChunkAfterPointMoved[ EWDir##dir ].first;                  \
        const auto     index                    = horizontalIndexChunkAfterPointMoved[ EWDir##dir ].second;                 \
        if ( !BlockAt( horizontalIndexChunkAfterPointMoved[ EWDir##dir ], 0 ).Transparent( ) )                              \
        {                                                                                                                   \
            chunk->m_NeighborTransparency[ index ] ^= oppositeDirectionBit;                                                 \
            chunk->UpdateAmbientOcclusionAt( index );                                                                       \
                                                                                                                            \
            /* Within visible range */                                                                                      \
            if constexpr ( EWDir##dir <= EWDirLeft )                                                                        \
                chunk->m_VisibleFacesCount += faceCountDiff;                                                                \
            /* regenerate other chunks faces */                                                                             \
            if ( chunk != this ) chunk->SyncChunkFromDirection( this, EWDir##dir ^ 0b1, true );                             \
        }                                                                                                                   \
                                                                                                                            \
        if ( !BlockAt( horizontalIndexChunkAfterPointMoved[ EWDir##dir ], dirUpFaceOffset ).Transparent( ) )                \
        {                                                                                                                   \
            chunk->m_NeighborTransparency[ index + dirUpFaceOffset ] ^= oppositeDirectionDownBit;                           \
            chunk->UpdateAmbientOcclusionAt( index + dirUpFaceOffset );                                                     \
                                                                                                                            \
            /* regenerate other chunks faces */                                                                             \
            if ( chunk != this ) chunk->SyncChunkFromDirection( this, EWDir##dir ^ 0b1, true );                             \
        }                                                                                                                   \
                                                                                                                            \
        if ( !BlockAt( horizontalIndexChunkAfterPointMoved[ EWDir##dir ], dirDownFaceOffset ).Transparent( ) )              \
        {                                                                                                                   \
            chunk->m_NeighborTransparency[ index + dirDownFaceOffset ] ^= oppositeDirectionUpBit;                           \
            chunk->UpdateAmbientOcclusionAt( index + dirDownFaceOffset );                                                   \
                                                                                                                            \
            /* regenerate other chunks faces */                                                                             \
            if ( chunk != this ) chunk->SyncChunkFromDirection( this, EWDir##dir ^ 0b1, true );                             \
        }                                                                                                                   \
    }

        UpdateAlongDir( Front );
        UpdateAlongDir( Back );
        UpdateAlongDir( Right );
        UpdateAlongDir( Left );

        UpdateAlongDir( FrontRight );
        UpdateAlongDir( FrontLeft );
        UpdateAlongDir( BackRight );
        UpdateAlongDir( BackLeft );

#undef UpdateAlongDir

#ifndef NDEBUG

        int checkingFaceCount = 0;
        for ( int i = 0; i < ChunkVolume; ++i )
            checkingFaceCount += std::popcount( m_NeighborTransparency[ i ] & DirFaceMask );
        assert( m_VisibleFacesCount == checkingFaceCount );
#endif

        // Update self ambient occlusion
        UpdateAmbientOcclusionAt( blockIndex );
    }

    return true;
}

bool
RenderableChunk::SyncChunkFromDirection( RenderableChunk* other, int fromDir, bool changes )
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

void
RenderableChunk::UpdateFacesAmbientOcclusion( FaceVertexAmbientOcclusionData& metaData, std::array<bool, 8> sideTransparency )
{
#define GET_STRENGTH( inx1, inx2, inx3 ) sideTransparency[ inx1 ] && sideTransparency[ inx3 ] ? 0 : ( 3 - (int) sideTransparency[ inx1 ] - (int) sideTransparency[ inx2 ] - (int) sideTransparency[ inx3 ] )
    metaData.data[ 0 ] = GET_STRENGTH( 0, 1, 2 );
    metaData.data[ 1 ] = GET_STRENGTH( 2, 3, 4 );
    metaData.data[ 2 ] = GET_STRENGTH( 4, 5, 6 );
    metaData.data[ 3 ] = GET_STRENGTH( 6, 7, 0 );
#undef GET_STRENGTH
}

void
RenderableChunk::UpdateAmbientOcclusionAt( uint32_t index )
{

    if ( const auto neighborTransparency = m_NeighborTransparency[ index ] )
    {
        // for ambient occlusion
#define ToNotBoolArray( A, B, C, D, E, F, G, H )                                                       \
    {                                                                                                  \
        !bool( A ), !bool( B ), !bool( C ), !bool( D ), !bool( E ), !bool( F ), !bool( G ), !bool( H ) \
    }

#define PassNeighborTransparency( D1, D2, D3, D4, D5, D6, D7, D8 ) \
    ToNotBoolArray( neighborTransparency& Dir##D1##Bit,            \
                    neighborTransparency& Dir##D2##Bit,            \
                    neighborTransparency& Dir##D3##Bit,            \
                    neighborTransparency& Dir##D4##Bit,            \
                    neighborTransparency& Dir##D5##Bit,            \
                    neighborTransparency& Dir##D6##Bit,            \
                    neighborTransparency& Dir##D7##Bit,            \
                    neighborTransparency& Dir##D8##Bit )

        if ( neighborTransparency & DirFrontBit )
            UpdateFacesAmbientOcclusion( m_VertexMetaData[ index ].faceVertexMetaData[ DirFront ].ambientOcclusionData, PassNeighborTransparency( FrontRight, FrontRightDown, FrontDown, FrontLeftDown, FrontLeft, FrontLeftUp, FrontUp, FrontRightUp ) );
        if ( neighborTransparency & DirBackBit )
            UpdateFacesAmbientOcclusion( m_VertexMetaData[ index ].faceVertexMetaData[ DirBack ].ambientOcclusionData, PassNeighborTransparency( BackLeft, BackLeftDown, BackDown, BackRightDown, BackRight, BackRightUp, BackUp, BackLeftUp ) );
        if ( neighborTransparency & DirRightBit )
            UpdateFacesAmbientOcclusion( m_VertexMetaData[ index ].faceVertexMetaData[ DirRight ].ambientOcclusionData, PassNeighborTransparency( BackRight, BackRightDown, RightDown, FrontRightDown, FrontRight, FrontRightUp, RightUp, BackRightUp ) );
        if ( neighborTransparency & DirLeftBit )
            UpdateFacesAmbientOcclusion( m_VertexMetaData[ index ].faceVertexMetaData[ DirLeft ].ambientOcclusionData, PassNeighborTransparency( FrontLeft, FrontLeftDown, LeftDown, BackLeftDown, BackLeft, BackLeftUp, LeftUp, FrontLeftUp ) );
        if ( neighborTransparency & DirUpBit )
            UpdateFacesAmbientOcclusion( m_VertexMetaData[ index ].faceVertexMetaData[ DirUp ].ambientOcclusionData, PassNeighborTransparency( LeftUp, BackLeftUp, BackUp, BackRightUp, RightUp, FrontRightUp, FrontUp, FrontLeftUp ) );
        if ( neighborTransparency & DirDownBit )
            UpdateFacesAmbientOcclusion( m_VertexMetaData[ index ].faceVertexMetaData[ DirDown ].ambientOcclusionData, PassNeighborTransparency( LeftDown, FrontLeftDown, FrontDown, FrontRightDown, RightDown, BackRightDown, BackDown, BackLeftDown ) );
    }

#undef PassNeighborTransparency
#undef ToBoolArray
}


void
RenderableChunk::UpdateMetaDataAt( uint32_t index )
{
    UpdateAmbientOcclusionAt( index );

    // For greedy meshing
    const auto& textureIndices = Minecraft::GetInstance( ).GetBlockTextures( ).GetTextureIndices( m_Blocks[ index ] );
    for ( int i = 0; i < CubeDirection::DirSize; ++i )
        m_VertexMetaData[ index ].faceVertexMetaData[ i ].textureID = textureIndices[ i ];
}

/*
 *
 * From https://github.com/roboleary/GreedyMesh/blob/master/src/mygame/Main.java
 * with multiple buf fixes
 *
 * */
std::array<std::vector<std::array<glm::ivec3, 4>>, CubeDirection::DirSize>
RenderableChunk::GenerateGreedyMesh( )
{
    static constexpr std::array<int, 3> dims { SectionUnitLength, ChunkMaxHeight, SectionUnitLength };

    std::array<std::vector<std::array<glm::ivec3, 4>>, CubeDirection::DirSize> faces { };

    /*
     * These are just working variables for the algorithm - almost all taken
     * directly from Mikola Lysenko's javascript implementation.
     */
    int i, j, k, l, w, h, u, v, n, side = 0;

    std::array<int, 3> x { 0, 0, 0 };
    std::array<int, 3> q { 0, 0, 0 };
    std::array<int, 3> du { 0, 0, 0 };
    std::array<int, 3> dv { 0, 0, 0 };

    /*
     * We create a mask - this will contain the groups of matching voxel faces
     * as we proceed through the chunk in 6 directions - once for each face.
     */
    static constexpr FaceVertexMetaData   emptyFace { .textureID = -1 };
    std::unique_ptr<FaceVertexMetaData[]> mask = std::make_unique<FaceVertexMetaData[]>( SectionUnitLength * ChunkMaxHeight );

    /**
     * We start with the lesser-spotted boolean for-loop (also known as the old flippy floppy).
     *
     * The variable backFace will be TRUE on the first iteration and FALSE on the second - this allows
     * us to track which direction the indices should run during creation of the quad.
     *
     * This loop runs twice, and the inner loop 3 times - totally 6 iterations - one for each
     * voxel face.
     */
    for ( bool backFace = true, b = false; b != backFace; backFace = backFace && b, b = !b )
    {

        /*
         * We sweep over the 3 dimensions - most of what follows is well described by Mikola Lysenko
         * in his post - and is ported from his Javascript implementation.  Where this implementation
         * diverges, I've added commentary.
         */
        for ( int d = 0; d < 3; d++ )
        {

            u = ( d + 1 ) % 3;
            v = ( d + 2 ) % 3;

            x[ 0 ] = 0;
            x[ 1 ] = 0;
            x[ 2 ] = 0;

            // Move along d axis
            q[ 0 ] = 0;
            q[ 1 ] = 0;
            q[ 2 ] = 0;
            q[ d ] = 1;

            /*
             * Here we're keeping track of the side that we're meshing.
             */
            if ( d == 0 )
            {
                side = backFace ? DirBack : DirFront;

            } else if ( d == 1 )
            {
                side = backFace ? DirDown : DirUp;

            } else if ( d == 2 )
            {
                side = backFace ? DirLeft : DirRight;
            }

            /*
             * We move through the dimension from front to back
             */
            for ( x[ d ] = -1; x[ d ] < dims[ d ]; )
            {

                /*
                 * -------------------------------------------------------------------
                 *   We compute the mask
                 * -------------------------------------------------------------------
                 */
                n = 0;

                for ( x[ v ] = 0; x[ v ] < dims[ v ]; x[ v ]++ )
                {

                    for ( x[ u ] = 0; x[ u ] < dims[ u ]; x[ u ]++ )
                    {

                        /*
                         * Here we retrieve two voxel faces for comparison.
                         */

                        const BlockCoordinate block1Coordinate = MakeMinecraftCoordinate( x[ 0 ], x[ 1 ], x[ 2 ] );
                        const BlockCoordinate block2Coordinate = MakeMinecraftCoordinate( x[ 0 ] + q[ 0 ], x[ 1 ] + q[ 1 ], x[ 2 ] + q[ 2 ] );

                        const auto block1 = GetBlockIndex( block1Coordinate );
                        const auto block2 = GetBlockIndex( block2Coordinate );

                        bool face1Visible = x[ d ] >= 0 && m_NeighborTransparency[ block1 ] & ( 1 << side );
                        bool face2Visible = x[ d ] < dims[ d ] - 1 && m_NeighborTransparency[ block2 ] & ( 1 << side );

                        if ( !backFace && face1Visible )
                            mask[ n++ ] = m_VertexMetaData[ block1 ].faceVertexMetaData[ side ];
                        else if ( backFace && face2Visible )
                            mask[ n++ ] = m_VertexMetaData[ block2 ].faceVertexMetaData[ side ];
                        else
                            mask[ n++ ] = emptyFace;   // face covered, ont visible
                    }
                }

                x[ d ]++;

                /*
                 * Now we generate the mesh for the mask
                 */
                n = 0;

                for ( j = 0; j < dims[ v ]; j++ )
                {

                    for ( i = 0; i < dims[ u ]; )
                    {

                        if ( mask[ n ].textureID != emptyFace.textureID )
                        {

                            /*
                             * We compute the width
                             */
                            for ( w = 1; i + w < dims[ u ] && mask[ n + w ].textureID != -1 && mask[ n + w ] == mask[ n ]; w++ ) { }

                            /*
                             * Then we compute height
                             */
                            bool done = false;

                            for ( h = 1; j + h < dims[ v ]; h++ )
                            {

                                for ( k = 0; k < w; k++ )
                                {

                                    if ( mask[ n + k + h * dims[ u ] ].textureID == -1 || mask[ n + k + h * dims[ u ] ] != mask[ n ] )
                                    {
                                        done = true;
                                        break;
                                    }
                                }

                                if ( done ) { break; }
                            }

                            /*
                             * Add quad
                             */
                            x[ u ] = i;
                            x[ v ] = j;

                            du[ 0 ] = 0;
                            du[ 1 ] = 0;
                            du[ 2 ] = 0;
                            du[ u ] = w;

                            dv[ 0 ] = 0;
                            dv[ 1 ] = 0;
                            dv[ 2 ] = 0;
                            dv[ v ] = h;

                            faces[ side ].push_back( std::array<glm::ivec3, 4> {
                                glm::ivec3 {                    x[ 0 ],                     x[ 1 ],                     x[ 2 ]}, /* Bottom Left */
                                glm::ivec3 {          x[ 0 ] + du[ 0 ],           x[ 1 ] + du[ 1 ],           x[ 2 ] + du[ 2 ]}, /* Top Left */
                                glm::ivec3 {x[ 0 ] + du[ 0 ] + dv[ 0 ], x[ 1 ] + du[ 1 ] + dv[ 1 ], x[ 2 ] + du[ 2 ] + dv[ 2 ]}, /* Top Right */
                                glm::ivec3 {          x[ 0 ] + dv[ 0 ],           x[ 1 ] + dv[ 1 ],           x[ 2 ] + dv[ 2 ]}  /* Bottom Right */
                            } );


                            /*
                             * We zero out the mask
                             */
                            for ( l = 0; l < h; ++l )
                            {

                                for ( k = 0; k < w; ++k )
                                {
                                    mask[ n + k + l * dims[ u ] ].textureID = -1;
                                }
                            }

                            /*
                             * And then finally increment the counters and continue
                             */
                            i += w;
                            n += w;

                        } else
                        {

                            i++;
                            n++;
                        }
                    }
                }
            }
        }
    }

    /*
     * for javascript debug purpose
     * https://github.com/mikolalysenko/mikolalysenko.github.com/tree/gh-pages/MinecraftMeshes
     *
     * */
    //    using Log = LoggerBase<false, false, false>;
    //
    //
    //    for ( const auto& coors : faces )
    //    {
    //        Log::getInstance( ).LogLine( "quads.push([" );
    //
    //        for ( const auto& vert : coors )
    //        {
    //            Log::getInstance( ).LogLine( "[", vert.x, ",", vert.y, ",", vert.z, "]," );
    //        }
    //
    //        Log::getInstance( ).LogLine( " ]);" );
    //    }

    return faces;
}
