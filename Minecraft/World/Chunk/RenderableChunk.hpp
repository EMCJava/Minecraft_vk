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

    int m_VisibleFacesCount = 0;

    std::array<RenderableChunk*, EightWayDirectionSize> m_NearChunks { };
    uint8_t                                             m_EmptySlot = ( 1 << EightWayDirectionSize ) - 1;

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
    void UpdateNeighborAt( uint32_t index );
    void RegenerateVisibleFaces( );

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
    bool                 SyncChunkFromDirection( RenderableChunk* other, EightWayDirection fromDir, bool changes = false );

    /*
     *
     * Access tools
     *
     * */
    bool SetBlock( const BlockCoordinate& blockCoordinate, const Block& block );

    inline auto GetNeighborTransparency( uint32_t index )
    {
        assert( m_NeighborTransparency != nullptr );
        return m_NeighborTransparency[ index ];
    }

    inline void DeleteCache( )
    {
        delete[] m_NeighborTransparency;
        m_NeighborTransparency = nullptr;

        m_VisibleFacesCount = 0;
    }

    inline bool     NeighborCompleted( ) const { return m_EmptySlot == 0; }
    inline uint32_t GetIndexBufferSize( ) const { return m_IndexBufferSize; }
};


#endif   // MINECRAFT_VK_RENDERABLECHUNK_HPP
