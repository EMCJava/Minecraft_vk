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

#include <Minecraft/util/Tickable.hpp>
#include <deque>

template <typename VertexTy, typename IndexTy>
class ChunkRenderBuffers : public Tickable
    , public Singleton<ChunkRenderBuffers<VertexTy, IndexTy>>
{

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

public:
    struct SuitableAllocation {
        BufferChunk*       targetChunk { };
        SingleBufferRegion region { };

        friend std::ostream& operator<<( std::ostream& o, const SuitableAllocation& sa )
        {
            o << "[ " << sa.targetChunk << "] : (" << sa.region.vertexStartingOffset << ", " << sa.region.vertexSize << ", " << sa.region.indexStartingOffset << ", " << sa.region.indexSize << ')' << std::flush;
            return o;
        }
    };

private:
    std::recursive_mutex    buffersMutex { };
    std::deque<BufferChunk> m_Buffers;

    std::mutex                                     m_PendingErasesLock;
    std::deque<std::pair<SuitableAllocation, int>> m_PendingErases;

    void GrowCapacity( );

public:
    explicit ChunkRenderBuffers( )
        : allocator( VulkanAPI::GetInstance( ).getMemoryAllocator( ) )
    { }
    ~ChunkRenderBuffers( );

    SuitableAllocation CreateBuffer( uint32_t vertexDataSize, uint32_t indexDataSize );
    void               DeleteBuffer( const ChunkRenderBuffers::SuitableAllocation& allocation );
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

    inline auto& GetPendingErasesLock( ) { return m_PendingErasesLock; }

    inline void Tick( float deltaTime )
    {
        std::lock_guard<std::mutex> lock( m_PendingErasesLock );
        while ( m_PendingErases.size( ) > 0 )
        {
            auto& front = m_PendingErases.front( );

            if ( front.second-- > 0 )
            {
                DeleteBuffer( front.first );
                m_PendingErases.pop_front( );
            }
        }
    }


    friend class MainApplication;
};

#include "ChunkRenderBuffers_Impl.hpp"

#endif   // MINECRAFT_VK_MINECRAFT_WORLD_CHUNK_CHUNKRENDERBUFFERS_HPP
