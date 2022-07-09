//
// Created by loys on 7/7/22.
//

#ifndef MINECRAFT_VK_BUFFERMETA_HPP
#define MINECRAFT_VK_BUFFERMETA_HPP

#include <Graphic/Vulkan/VulkanAPI.hpp>

class BufferMeta
{

    VmaAllocator  allocator { };
    vk::Buffer    buffer { };
    VmaAllocation allocation { };

public:
    BufferMeta( )
        : allocator( VulkanAPI::GetInstance( ).getMemoryAllocator( ) )
    { }

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

    void CopyFromBuffer( const BufferMeta& bufferData, const vk::ArrayProxy<const vk::BufferCopy>& dataRegion, VulkanAPI& api )
    {
        static std::mutex           copy_buffer_lock;
        std::lock_guard<std::mutex> guard( copy_buffer_lock );

        vk::CommandBufferAllocateInfo allocInfo { };
        allocInfo.setCommandPool( *api.GetTransferCommandPool( ) );
        allocInfo.setLevel( vk::CommandBufferLevel::ePrimary );
        allocInfo.setCommandBufferCount( 1 );

        auto  commandBuffers = api.getLogicalDevice( ).allocateCommandBuffersUnique( allocInfo );
        auto& commandBuffer  = commandBuffers.begin( )->get( );

        commandBuffer.begin( { vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );
        commandBuffer.copyBuffer( bufferData.buffer, buffer, dataRegion );
        commandBuffer.end( );

        const auto& transferFamilyIndices = api.GetTransferFamilyIndices( );
        auto        transferQueue         = api.getLogicalDevice( ).getQueue( transferFamilyIndices.first, transferFamilyIndices.second );

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers( commandBuffer );
        transferQueue.submit( submitInfo, nullptr );
        transferQueue.waitIdle( );
    }
};


#endif   // MINECRAFT_VK_BUFFERMETA_HPP
