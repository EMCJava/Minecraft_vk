//
// Created by loys on 5/24/22.
//

#ifndef MINECRAFT_VK_RENDERABLECHUNK_HPP
#define MINECRAFT_VK_RENDERABLECHUNK_HPP

#include "Chunk.hpp"
#include "ChunkRenderBuffers.hpp"

#include <Minecraft/util/MinecraftType.h>

#include <Utility/Thread/ThreadPool.hpp>

#include <Graphic/Vulkan/BufferMeta.hpp>
#include <Graphic/Vulkan/VulkanAPI.hpp>

#include <unordered_map>

using ChunkSolidBuffer = ChunkRenderBuffers<DataType::TexturedVertex, IndexBufferType>;

using FaceVertexAmbientOcclusionData = uint8_t;
// union FaceVertexAmbientOcclusionData
//{
//     uint32_t uuid = 0;
//     /* First two bit are for brightness */
//     uint8_t data[ 4 ];
// };

inline auto
GetAmbientOcclusionDataAt( FaceVertexAmbientOcclusionData data, int index )
{
    return (data >> ( index * 2 )) & 0b11;
}

inline auto
SetAmbientOcclusionDataAt( FaceVertexAmbientOcclusionData& data, FaceVertexAmbientOcclusionData newData, int index )
{
    return data |= newData << ( index * 2 );
}

struct FaceVertexMetaData {

    bool                           quadFlipped = false;
    FaceVertexAmbientOcclusionData ambientOcclusionData { };
    int32_t                        textureID   = 0;

    inline bool operator==( const FaceVertexMetaData& other ) const
    {
        return textureID == other.textureID && ambientOcclusionData == other.ambientOcclusionData && quadFlipped == other.quadFlipped;
    }

    inline bool operator!=( const FaceVertexMetaData& other ) const
    {
        return !( *this == other );
    }
};

struct CubeVertexMetaData {

    FaceVertexMetaData faceVertexMetaData[ CubeDirection::DirSize ];
};

struct GreedyMeshFace {

    glm::ivec3 offset, scale;
    glm::ivec2 textureScale;

    GreedyMeshFace( glm::ivec3 offset, glm::ivec3 scale, glm::ivec2 textureScale )
        : offset( offset )
        , scale( scale )
        , textureScale( textureScale )
    { }
};

struct GreedyMeshCollection {

    std::vector<GreedyMeshFace> faces;
};

namespace std
{

template <>
struct hash<FaceVertexMetaData> {
    std::size_t operator( )( const FaceVertexMetaData& k ) const
    {
        // Compute individual hash values for first,
        // second and third and combine them using XOR
        // and bit shifting:

        return ( (std::size_t) k.textureID << sizeof( FaceVertexAmbientOcclusionData ) )
            + k.ambientOcclusionData;
    }
};

}   // namespace std

class RenderableChunk : public Chunk
{
protected:
    /*
     *
     * Chunk detail
     *
     * */
    uint32_t* m_NeighborTransparency { };
    static_assert( IntLog<DirBitSize - 1, 2>::value < ( sizeof( std::remove_pointer_t<decltype( m_NeighborTransparency )> ) << 3 ) );

    int                 m_VisibleFacesCount = 0;
    CubeVertexMetaData* m_VertexMetaData { };

    std::array<RenderableChunk*, EightWayDirectionSize> m_NearChunks { };
    uint8_t                                             m_EmptySlot = ( 1 << EightWayDirectionSize ) - 1;

    std::array<std::pair<RenderableChunk*, uint32_t>, EightWayDirectionSize>
    GetHorizontalChunkAfterPointMoved( uint32_t index );

    /*
     *
     * Render buffer
     *
     * */
    uint32_t m_IndexBufferSize { };

    ChunkSolidBuffer::SuitableAllocation m_BufferAllocation;

    /*
     *
     * Can only be used when surrounding chunk is loaded
     *
     * */

    // return true if quad needed to be flipped for better viewing
    bool UpdateFacesAmbientOcclusion( FaceVertexAmbientOcclusionData& metaData, std::array<bool, 8> sideTransparency );
    void UpdateAmbientOcclusionAt( uint32_t index );
    void UpdateMetaDataAt( uint32_t index );
    void UpdateNeighborAt( uint32_t index );
    void RegenerateVisibleFaces( );

    std::array<std::unordered_map<FaceVertexMetaData, GreedyMeshCollection>, CubeDirection::DirSize> GenerateGreedyMesh( );

public:
    explicit RenderableChunk( class MinecraftWorld* world )
        : Chunk( world )
    { }
    ~RenderableChunk( )
    {
        DeleteCache( );

        if ( m_BufferAllocation.targetChunk != nullptr )
        {
            ChunkSolidBuffer::GetInstance( ).DelayedDeleteBuffer( m_BufferAllocation );
        }

        for ( int i = 0; i < EightWayDirectionSize; ++i )
            if ( m_NearChunks[ i ] != nullptr ) m_NearChunks[ i ]->SyncChunkFromDirection( nullptr, static_cast<EightWayDirection>( i ^ 0b1 ) );
    }

    bool initialized  = false;
    bool initializing = false;

    void ResetRenderBuffer( );
    void GenerateRenderBuffer( );

    // return true if target chunk become complete
    std::recursive_mutex m_SyncMutex { };
    bool                 SyncChunkFromDirection( RenderableChunk* other, int fromDir, bool changes = false );

    /*
     *
     * Access tools
     *
     * */
    bool SetBlock( const BlockCoordinate& blockCoordinate, const Block& block );

    inline auto GetNeighborTransparency( uint32_t index ) const
    {
        assert( m_NeighborTransparency != nullptr );
        return m_NeighborTransparency[ index ];
    }

    inline const auto& GetCubeVertexMetaData( uint32_t index ) const
    {
        assert( m_VertexMetaData != nullptr );
        return m_VertexMetaData[ index ];
    }

    inline bool IsCacheReady( ) { return m_NeighborTransparency != nullptr && m_VertexMetaData != nullptr; }

    inline void DeleteCache( )
    {
        delete[] m_NeighborTransparency;
        m_NeighborTransparency = nullptr;

        delete[] m_VertexMetaData;
        m_VertexMetaData = nullptr;

        m_VisibleFacesCount = 0;
    }

    inline bool     NeighborCompleted( ) const { return m_EmptySlot == 0; }
    inline uint32_t GetIndexBufferSize( ) const { return m_IndexBufferSize; }

    size_t GetObjectSize( ) const;
};


#endif   // MINECRAFT_VK_RENDERABLECHUNK_HPP
