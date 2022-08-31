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
    uint8_t* m_BlockFaces { };
    int      m_VisibleFacesCount = 0;

    std::array<RenderableChunk*, DirHorizontalSize> m_NearChunks { };
    uint8_t                                         m_EmptySlot = ( 1 << DirHorizontalSize ) - 1;

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
    void RegenerateVisibleFacesAt( const uint32_t index );
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

        for ( int i = 0; i < DirHorizontalSize; ++i )
            if ( m_NearChunks[ i ] != nullptr ) m_NearChunks[ i ]->SyncChunkFromDirection( nullptr, static_cast<Direction>( i ^ 0b1 ) );
    }

    bool initialized  = false;
    bool initializing = false;

    void ResetRenderBuffer( );
    void GenerateRenderBuffer( );

    // return true if target chunk become complete
    std::recursive_mutex m_SyncMutex { };
    bool                 SyncChunkFromDirection( RenderableChunk* other, Direction fromDir, bool changes = false );

    /*
     *
     * Access tools
     *
     * */
    bool SetBlock( const BlockCoordinate& blockCoordinate, const Block& block );

    inline void DeleteCache( )
    {
        delete[] m_BlockFaces;
        m_BlockFaces = nullptr;

        m_VisibleFacesCount = 0;
    }

    inline uint32_t GetIndexBufferSize( ) const { return m_IndexBufferSize; }
};


#endif   // MINECRAFT_VK_RENDERABLECHUNK_HPP
