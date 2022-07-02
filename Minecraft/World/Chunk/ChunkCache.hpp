//
// Created by loys on 5/24/22.
//

#ifndef MINECRAFT_VK_CHUNKCACHE_HPP
#define MINECRAFT_VK_CHUNKCACHE_HPP

#include "Chunk.hpp"

#include <Minecraft/util/MinecraftType.h>

#include <Utility/Thread/ThreadPool.hpp>

#include <Graphic/Vulkan/VulkanAPI.hpp>

#include <unordered_map>

class ChunkCache
{
private:
    std::unique_ptr<VulkanAPI::VKMBufferMeta> m_VertexBuffer;
    std::unique_ptr<VulkanAPI::VKMBufferMeta> m_IndexBuffer;
    uint32_t                                  m_IndexBufferSize { };

public:
    Chunk chunk;
    bool  initialized  = false;
    bool  initializing = false;

    void ResetLoad( );

    const VulkanAPI::VKMBufferMeta& GetVertexBuffer( ) const { return *m_VertexBuffer; }
    const VulkanAPI::VKMBufferMeta& GetIndexBuffer( ) const { return *m_IndexBuffer; }
    inline uint32_t                 GetIndexBufferSize( ) const { return m_IndexBufferSize; }
};


#endif   // MINECRAFT_VK_CHUNKCACHE_HPP
