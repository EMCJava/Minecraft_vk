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
    VulkanAPI::VKBufferMeta m_VertexBuffer;
    VulkanAPI::VKBufferMeta m_IndexBuffer;

public:
    Chunk chunk;
    bool  initialized  = false;
    bool  initializing = false;

    void ResetLoad( );

    const VulkanAPI::VKBufferMeta& GetVertexBuffer( ) const { return m_VertexBuffer; }
    const VulkanAPI::VKBufferMeta& GetIndexBuffer( ) const { return m_IndexBuffer; }
};


#endif   // MINECRAFT_VK_CHUNKCACHE_HPP
