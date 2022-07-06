//
// Created by loys on 5/24/22.
//

#ifndef MINECRAFT_VK_CHUNKCACHE_HPP
#define MINECRAFT_VK_CHUNKCACHE_HPP

#include "Chunk.hpp"
#include "ChunkRenderBuffers.hpp"

#include <Minecraft/util/MinecraftType.h>

#include <Utility/Thread/ThreadPool.hpp>

#include <Graphic/Vulkan/VulkanAPI.hpp>

#include <unordered_map>

using ChunkSolidBuffer = ChunkRenderBuffers<DataType::ColoredVertex, IndexBufferType, SCVISCC>;

class ChunkCache
{
private:
    std::unique_ptr<VulkanAPI::VKMBufferMeta> m_VertexBuffer;
    std::unique_ptr<VulkanAPI::VKMBufferMeta> m_IndexBuffer;
    uint32_t                                  m_IndexBufferSize { };
    std::atomic_flag m_BufferReady            ATOMIC_FLAG_INIT;

public:
    Chunk chunk;
    bool  initialized  = false;
    bool  initializing = false;

    void ResetLoad( );
    void ResetModel( ChunkSolidBuffer& renderBuffers );

    bool                            IsBufferReady( ) const { return m_BufferReady.test( ); }
    const VulkanAPI::VKMBufferMeta& GetVertexBuffer( ) const { return *m_VertexBuffer; }
    const VulkanAPI::VKMBufferMeta& GetIndexBuffer( ) const { return *m_IndexBuffer; }
    inline uint32_t                 GetIndexBufferSize( ) const { return m_IndexBufferSize; }
};


#endif   // MINECRAFT_VK_CHUNKCACHE_HPP
