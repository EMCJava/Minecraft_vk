//
// Created by loys on 7/7/22.
//

#ifndef MINECRAFT_VK_BUFFERMETA_HPP
#define MINECRAFT_VK_BUFFERMETA_HPP

#include <Include/GraphicAPI.hpp>
#include <Include/vk_mem_alloc.h>

#include <fstream>
#include <mutex>

class BufferMeta
{

    VmaAllocator  allocator { };
    vk::Buffer    buffer { };
    VmaAllocation allocation { };

public:
    BufferMeta( ) = default;

    ~BufferMeta( )
    {
        DestroyBuffer( );
    }

    void DestroyBuffer( )
    {
        if ( buffer ) vmaDestroyBuffer( allocator, buffer, allocation );
        buffer = nullptr;
    }

    inline auto& GetBuffer( ) const
    {
        return buffer;
    }

    void Create( const vk::DeviceSize size, const vk::BufferUsageFlags usage, const VmaMemoryUsage& memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
                 const vk::SharingMode sharingMode = vk::SharingMode::eExclusive, const VmaAllocationCreateFlags& memoryFlag = 0 )
    {
        static std::mutex           create_buffer_lock;
        std::lock_guard<std::mutex> guard( create_buffer_lock );

        DestroyBuffer( );

        vk::BufferCreateInfo    bufferInfo { { }, size, usage, sharingMode };
        VmaAllocationCreateInfo allocInfo = { };
        allocInfo.usage                   = memoryUsage;
        allocInfo.flags                   = memoryFlag;
        // allocInfo.requiredFlags           = static_cast<uint32_t>( memoryProperties );

        vmaCreateBuffer( allocator, reinterpret_cast<const VkBufferCreateInfo*>( &bufferInfo ), &allocInfo, reinterpret_cast<VkBuffer*>( &buffer ), &allocation, nullptr );
        // vmaBindBufferMemory( allocator, allocation, buffer );
    }

    void writeBuffer( const void* writingData, size_t dataSize )
    {
        static std::mutex           map_buffer_lock;
        std::lock_guard<std::mutex> guard( map_buffer_lock );

        void* mappedData = nullptr;
        vmaMapMemory( allocator, allocation, &mappedData );
        memcpy( mappedData, writingData, dataSize );
        vmaUnmapMemory( allocator, allocation );
    }

    void writeBufferOffseted( const void* writingData, size_t dataSize, size_t offset )
    {
        void* mappedData = nullptr;
        vmaMapMemory( allocator, allocation, &mappedData );
        memcpy( (char*) mappedData + offset, writingData, dataSize );
        vmaUnmapMemory( allocator, allocation );
    }

    void CopyFromBuffer( const BufferMeta& bufferData, const vk::ArrayProxy<const vk::BufferCopy>& dataRegion, class VulkanAPI& api );

    void SetAllocator( VmaAllocator newAllocator = nullptr );

    friend std::ostream& operator<<( std::ostream& o, const BufferMeta& bm )
    {
        o << (VkBuffer) bm.buffer << std::flush;
        return o;
    }
};


#endif   // MINECRAFT_VK_BUFFERMETA_HPP
