//
// Created by samsa on 7/3/2022.
//

#ifndef MINECRAFT_VK_MINECRAFT_WORLD_CHUNK_CHUNKRENDERBUFFERS_HPP
#define MINECRAFT_VK_MINECRAFT_WORLD_CHUNK_CHUNKRENDERBUFFERS_HPP

#include <Graphic/Vulkan/BufferMeta.hpp>
#include <Graphic/Vulkan/VulkanAPI.hpp>
#include <Include/vk_mem_alloc.h>
#include <Minecraft/util/MinecraftConstants.hpp>
#include <Utility/Math/Math.hpp>

#include <deque>

template <typename VertexTy, typename IndexTy>
class ChunkRenderBuffers
{
    static_assert( ScaleToSecond<sizeof( VertexTy ), sizeof( IndexTy )>( sizeof( VertexTy ) ) == sizeof( IndexTy ) );

public:
    struct SingleBufferRegion {
        uint32_t vertexStartingOffset;
        uint32_t vertexSize;

        uint32_t indexStartingOffset;
        uint32_t indexSize;

        inline void SetBufferAllocation( uint32_t startingPoint, uint32_t vertexDataSize, uint32_t indexDataSize )
        {
            vertexStartingOffset = startingPoint;
            vertexSize           = vertexDataSize;
            indexStartingOffset  = AlignTo<uint32_t>( startingPoint + vertexDataSize, sizeof( IndexTy ) );
            indexSize            = indexDataSize;
        }

        inline uint32_t GetOffsetDiff( )
        {
            assert( indexStartingOffset > vertexStartingOffset );
            return indexStartingOffset - vertexStartingOffset;
        }

        inline uint32_t GetVertexEndingPoint( ) const { return vertexStartingOffset + vertexSize; }
        inline uint32_t GetIndexEndingPoint( ) const { return indexStartingOffset + indexSize; }

        inline uint32_t GetStartingPoint( ) const { return vertexStartingOffset; }
        inline uint32_t GetEndingPoint( ) const { return indexStartingOffset + indexSize; }
        inline uint32_t GetBufferSize( ) const { return vertexSize + indexSize; }
        inline uint32_t GetTotalSize( ) const { return GetEndingPoint( ) - GetStartingPoint( ); }

        inline bool operator==( const SingleBufferRegion& other )
        {
            return vertexStartingOffset == other.vertexStartingOffset && vertexSize == other.vertexSize && indexStartingOffset == other.indexStartingOffset && indexSize == other.indexSize;
        }
    };

private:
    VmaAllocator allocator { };

    struct BufferChunk {
        VmaAllocator allocator { };

        explicit BufferChunk( )
            : allocator( VulkanAPI::GetInstance( ).getMemoryAllocator( ) )
        { }

        ~BufferChunk( )
        {
            vmaDestroyBuffer( allocator, buffer, bufferAllocation );
        }

        vk::Buffer    buffer { };
        VmaAllocation bufferAllocation { };

        std::vector<vk::DrawIndexedIndirectCommand> indirectCommands;

        bool       shouldUpdateIndirectDrawBuffers = false;
        std::mutex indirectDrawBuffersMutex { };
        uint32_t   indirectDrawBufferSize = 0;
        BufferMeta stagingBuffer;
        BufferMeta indirectDrawBuffers;

        void UpdateIndirectDrawBuffers( );

        std::vector<SingleBufferRegion> m_DataSlots;
    };

    std::recursive_mutex    buffersMutex { };
    std::deque<BufferChunk> m_Buffers;

    void GrowCapacity( );

public:
    explicit ChunkRenderBuffers( )
        : allocator( VulkanAPI::GetInstance( ).getMemoryAllocator( ) )
    { }

    ~ChunkRenderBuffers( );

    struct SuitableAllocation {
        BufferChunk*       targetChunk { };
        SingleBufferRegion region { };
    };

    SuitableAllocation CreateBuffer( uint32_t vertexDataSize, uint32_t indexDataSize );
    SuitableAllocation AlterBuffer( const ChunkRenderBuffers::SuitableAllocation& allocation, uint32_t vertexDataSize, uint32_t indexDataSize );
    void               CopyBuffer( SuitableAllocation allocation, void* vertexBuffer, void* indexBuffer );

    void UpdateAllIndirectDrawBuffers( )
    {
        for ( auto& chunk : m_Buffers )
        {
            chunk.UpdateIndirectDrawBuffers( );
        }
    }

    void Clean( )
    {
        m_Buffers.clear( );
    }

    friend class MainApplication;
};

#include "ChunkRenderBuffers_Impl.hpp"

#endif   // MINECRAFT_VK_MINECRAFT_WORLD_CHUNK_CHUNKRENDERBUFFERS_HPP
