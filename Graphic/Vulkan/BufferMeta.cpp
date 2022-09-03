//
// Created by loys on 7/7/22.
//

#include "BufferMeta.hpp"
#include <Graphic/Vulkan/VulkanAPI.hpp>

void
BufferMeta::CopyFromBuffer( const BufferMeta& bufferData, const vk::ArrayProxy<const vk::BufferCopy>& dataRegion, VulkanAPI& api )
{
    static std::mutex           copy_buffer_lock;
    std::lock_guard<std::mutex> guard( copy_buffer_lock );

    // this will lock the transfer queue
    const auto& transferFamilyIndicesResources = api.GetTransferFamilyIndices( );
    auto        transferQueue                  = api.getLogicalDevice( ).getQueue( transferFamilyIndicesResources.resources.first, transferFamilyIndicesResources.resources.second );

    vk::CommandBufferAllocateInfo allocInfo { };
    allocInfo.setCommandPool( *api.GetTransferCommandPool( ) );
    allocInfo.setLevel( vk::CommandBufferLevel::ePrimary );
    allocInfo.setCommandBufferCount( 1 );

    auto  commandBuffers = api.getLogicalDevice( ).allocateCommandBuffersUnique( allocInfo );
    auto& commandBuffer  = commandBuffers.begin( )->get( );

    commandBuffer.begin( { vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );
    commandBuffer.copyBuffer( bufferData.buffer, buffer, dataRegion );
    commandBuffer.end( );

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers( commandBuffer );
    transferQueue.submit( submitInfo, nullptr );
    transferQueue.waitIdle( );
}

void
BufferMeta::SetAllocator( VmaAllocator newAllocator )
{
    allocator = newAllocator ? newAllocator : VulkanAPI::GetInstance( ).getMemoryAllocator( );
}

