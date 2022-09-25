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

    auto AddFace = [ indexOffset, faceAdded = 0, chunkVerticesPtr = chunkVertices.get( ), chunkIndicesPtr = chunkIndices.get( ) ]( const std::array<DataType::TexturedVertex, FaceVerticesCount>& vertexArray, const glm::vec3& offset, std::array<bool, 8> sideTransparency ) mutable {
        static constexpr auto faceShaderMultiplier = 1.0f / 3;

#define GET_STRENGTH( inx1, inx2, inx3 ) sideTransparency[ inx1 ] && sideTransparency[ inx3 ] ? 0 : ( 3 - (int) sideTransparency[ inx1 ] - (int) sideTransparency[ inx2 ] - (int) sideTransparency[ inx3 ] )
        std::array<int, FaceVerticesCount> faceAmbientOcclusionStrengths = { GET_STRENGTH( 0, 1, 2 ), GET_STRENGTH( 2, 3, 4 ), GET_STRENGTH( 4, 5, 6 ), GET_STRENGTH( 6, 7, 0 ) };
#undef GET_STRENGTH

        for ( int i = 0; i < FaceVerticesCount; ++i )
        {
            chunkVerticesPtr[ i ] = vertexArray[ i ];
            chunkVerticesPtr[ i ].pos += offset;
            chunkVerticesPtr[ i ].textureCoor_ColorIntensity.z *= 0.2f + faceAmbientOcclusionStrengths[ i ] * faceShaderMultiplier;
        }

        chunkVerticesPtr += FaceVerticesCount;

        if ( faceAmbientOcclusionStrengths[ 3 ] + faceAmbientOcclusionStrengths[ 1 ] > faceAmbientOcclusionStrengths[ 0 ] + faceAmbientOcclusionStrengths[ 2 ] )
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
                        AddFace( textures[ DirFront ], offset, PassNeighborTransparency( FrontRight, FrontRightDown, FrontDown, FrontLeftDown, FrontLeft, FrontLeftUp, FrontUp, FrontRightUp ) );
                    if ( neighborTransparency & DirBackBit )
                        AddFace( textures[ DirBack ], offset, PassNeighborTransparency( BackLeft, BackLeftDown, BackDown, BackRightDown, BackRight, BackRightUp, BackUp, BackLeftUp ) );
                    if ( neighborTransparency & DirRightBit )
                        AddFace( textures[ DirRight ], offset, PassNeighborTransparency( BackRight, BackRightDown, RightDown, FrontRightDown, FrontRight, FrontRightUp, RightUp, BackRightUp ) );
                    if ( neighborTransparency & DirLeftBit )
                        AddFace( textures[ DirLeft ], offset, PassNeighborTransparency( FrontLeft, FrontLeftDown, LeftDown, BackLeftDown, BackLeft, BackLeftUp, LeftUp, FrontLeftUp ) );
                    if ( neighborTransparency & DirUpBit )
                        AddFace( textures[ DirUp ], offset, PassNeighborTransparency( LeftUp, BackLeftUp, BackUp, BackRightUp, RightUp, FrontRightUp, FrontUp, FrontLeftUp ) );
                    if ( neighborTransparency & DirDownBit )
                        AddFace( textures[ DirDown ], offset, PassNeighborTransparency( LeftDown, FrontLeftDown, FrontDown, FrontRightDown, RightDown, BackRightDown, BackDown, BackLeftDown ) );
                }

#undef PassNeighborTransparency
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

        const auto* blockPtr     = m_Blocks.get( ) + blockIndex;
        auto* const blockFacePtr = m_NeighborTransparency + blockIndex;
        if ( !blockOnTop )
            if ( !blockPtr[ dirUpFaceOffset ].Transparent( ) )
            {
                blockFacePtr[ dirUpFaceOffset ] ^= DirDownBit;
                m_VisibleFacesCount += faceCountDiff;
            }

        if ( !blockOnBottom )
            if ( !blockPtr[ dirDownFaceOffset ].Transparent( ) )
            {
                blockFacePtr[ dirDownFaceOffset ] ^= DirUpBit;
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
            /* regenerate other chunks faces */                                                                             \
            if ( chunk != this ) chunk->SyncChunkFromDirection( this, EWDir##dir ^ 0b1, true );                             \
        }                                                                                                                   \
                                                                                                                            \
        if ( !BlockAt( horizontalIndexChunkAfterPointMoved[ EWDir##dir ], dirDownFaceOffset ).Transparent( ) )              \
        {                                                                                                                   \
            chunk->m_NeighborTransparency[ index + dirDownFaceOffset ] ^= oppositeDirectionUpBit;                           \
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