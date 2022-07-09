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

template <typename VertexTy, typename IndexTy, typename SizeConverter = ScaleToSecond_S<sizeof( VertexTy ), sizeof( IndexTy )>>
class ChunkRenderBuffers
{
    static_assert( ScaleToSecond<sizeof( VertexTy ), sizeof( IndexTy )>( sizeof( VertexTy ) ) == sizeof( IndexTy ) );

public:
    struct SingleBufferRegion {
        std::pair<uint32_t, uint32_t> vertex;
        std::pair<uint32_t, uint32_t> index;
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
            vmaDestroyBuffer( allocator, vertexBuffer, vertexAllocation );
            vmaDestroyBuffer( allocator, indexBuffer, indexAllocation );
        }

        vk::Buffer    vertexBuffer { };
        VmaAllocation vertexAllocation { };

        vk::Buffer    indexBuffer { };
        VmaAllocation indexAllocation { };

        std::vector<vk::DrawIndexedIndirectCommand> indirectCommands;

        bool       shouldUpdateIndirectDrawBuffers = false;
        std::mutex indirectDrawBuffersMutex { };
        uint32_t   indirectDrawBufferSize = 0;
        BufferMeta stagingBuffer;
        BufferMeta indirectDrawBuffers;

        void UpdateIndirectDrawBuffers( );

        std::vector<SingleBufferRegion> m_DataSlots;
        std::vector<SingleBufferRegion> m_EmptySlots;
    };

    std::deque<BufferChunk> m_Buffers;

    void GrowCapacity( );

    inline uint32_t
    VertexBufferToIndexBufferIndex( uint32_t index )
    {
        return ScaleToSecond<sizeof( IndexBufferType ), 1>( SizeConverter::ConvertToSecond( index ) );
    }

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
    void               CopyBuffer( SuitableAllocation allocation, void* vertexBuffer, void* indexBuffer );
    void               UpdateAllIndirectDrawBuffers( )
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
