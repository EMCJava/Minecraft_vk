//
// Created by samsa on 7/3/2022.
//

#ifndef MINECRAFT_VK_MINECRAFT_WORLD_CHUNK_CHUNKRENDERBUFFERS_IMPL_HPP
#define MINECRAFT_VK_MINECRAFT_WORLD_CHUNK_CHUNKRENDERBUFFERS_IMPL_HPP

#include <Minecraft/util/MinecraftConstants.hpp>

#include "ChunkRenderBuffers.hpp"
#include <cmath>

template <typename VertexTy, typename IndexTy, typename SizeConverter>
typename ChunkRenderBuffers<VertexTy, IndexTy, SizeConverter>::SuitableAllocation
ChunkRenderBuffers<VertexTy, IndexTy, SizeConverter>::CreateBuffer( uint32_t vertexDataSize, uint32_t indexDataSize )
{
    auto ratio = (float) sizeof( VertexTy ) / sizeof( IndexTy );
    assert( SizeConverter::ConvertToSecond( vertexDataSize ) >= indexDataSize );

    if ( m_Buffers.empty( ) )
    {
        assert( vertexDataSize < MaxMemoryAllocationPerVertexBuffer );
        GrowCapacity( );
    }

    std::vector<std::pair<SuitableAllocation, uint32_t>> suitableAllocations;
    for ( auto& bufferChunk : m_Buffers )
    {
        for ( int i = 0; i < bufferChunk.m_EmptySlots.size( ); ++i )
        {
            if ( bufferChunk.m_EmptySlots[ i ].vertex.second > vertexDataSize )
            {
                suitableAllocations.push_back( {
                    {&bufferChunk, bufferChunk.m_EmptySlots[ i ]},
                    i
                } );
                break;
            }
        }
    }

    if ( !suitableAllocations.empty( ) )
    {
        std::sort( suitableAllocations.begin( ), suitableAllocations.end( ), []( const auto& a, const auto& b ) {
            return a.first.region.vertex.second < b.first.region.vertex.second;
        } );

        auto& chunkUsing     = *suitableAllocations.begin( )->first.targetChunk;
        auto& slotIndexUsing = suitableAllocations.begin( )->second;
        auto& emptySlotUsing = chunkUsing.m_EmptySlots[ slotIndexUsing ];

        SingleBufferRegion newAllocation = emptySlotUsing;
        newAllocation.vertex.second      = vertexDataSize;
        newAllocation.index.second       = indexDataSize;
        chunkUsing.m_DataSlots.push_back( newAllocation );

        auto const sizeDiffer = emptySlotUsing.vertex.second - vertexDataSize;
        if ( sizeDiffer > 0 )
        {
            chunkUsing.m_EmptySlots.insert( chunkUsing.m_EmptySlots.begin( ) + slotIndexUsing + 1, SingleBufferRegion {
                                                                                                       {                            emptySlotUsing.vertex.first + vertexDataSize,                             sizeDiffer},
                                                                                                       {SCVISCC::ConvertToSecond( emptySlotUsing.vertex.first + vertexDataSize ), SCVISCC::ConvertToSecond( sizeDiffer )}
            } );
        }

        chunkUsing.m_EmptySlots.erase( chunkUsing.m_EmptySlots.begin( ) + slotIndexUsing );

        auto& newCommand = chunkUsing.indirectCommands.emplace_back( );

        newCommand.instanceCount = 1;
        newCommand.firstInstance = 0;   // chunkUsing.m_DataSlots.size( ) - 1;
        newCommand.firstIndex    = VertexBufferToIndexBufferIndex( newAllocation.vertex.first );
        newCommand.indexCount    = ScaleToSecond<sizeof( IndexBufferType ), 1>( indexDataSize );

        chunkUsing.shouldUpdateIndirectDrawBuffers = true;

        return { &chunkUsing, newAllocation };

    } else
    {
        assert( vertexDataSize < MaxMemoryAllocationPerVertexBuffer );
        GrowCapacity( );

        return CreateBuffer( vertexDataSize, indexDataSize );
    }
}

template <typename VertexTy, typename IndexTy, typename SizeConverter>
void
ChunkRenderBuffers<VertexTy, IndexTy, SizeConverter>::GrowCapacity( )
{
    using Usage = vk::BufferUsageFlagBits;

    auto& newBuffer = m_Buffers.emplace_back( );

    vk::BufferCreateInfo    bufferInfo { { }, MaxMemoryAllocationPerVertexBuffer, Usage::eVertexBuffer | Usage::eTransferDst, vk::SharingMode::eExclusive };
    VmaAllocationCreateInfo allocInfo = { };
    allocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.flags                   = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    vmaCreateBuffer( allocator, reinterpret_cast<const VkBufferCreateInfo*>( &bufferInfo ), &allocInfo, reinterpret_cast<VkBuffer*>( &newBuffer.vertexBuffer ), &newBuffer.vertexAllocation, nullptr );

    bufferInfo.setSize( MaxMemoryAllocationPerIndexBuffer ).setUsage( Usage::eIndexBuffer | Usage::eTransferDst );
    vmaCreateBuffer( allocator, reinterpret_cast<const VkBufferCreateInfo*>( &bufferInfo ), &allocInfo, reinterpret_cast<VkBuffer*>( &newBuffer.indexBuffer ), &newBuffer.indexAllocation, nullptr );

    newBuffer.m_EmptySlots.emplace_back( SingleBufferRegion {
        {0, MaxMemoryAllocationPerVertexBuffer}
    } );
}

template <typename VertexTy, typename IndexTy, typename SizeConverter>
ChunkRenderBuffers<VertexTy, IndexTy, SizeConverter>::~ChunkRenderBuffers( )
{
    Clean( );
}

template <typename VertexTy, typename IndexTy, typename SizeConverter>
void
ChunkRenderBuffers<VertexTy, IndexTy, SizeConverter>::CopyBuffer( ChunkRenderBuffers::SuitableAllocation allocation, void* vertexBuffer, void* indexBuffer )
{
    using Usage = vk::BufferUsageFlagBits;

    BufferMeta stagingBuffer;

    const vk::DeviceSize stagingSize = std::ceil( allocation.region.vertex.second + SCVISCC::ConvertToSecond( allocation.region.vertex.second ) );
    stagingBuffer.Create( stagingSize, Usage::eVertexBuffer | Usage::eTransferSrc );
    stagingBuffer.writeBuffer( vertexBuffer, allocation.region.vertex.second );
    stagingBuffer.writeBufferOffseted( indexBuffer, allocation.region.index.second, allocation.region.vertex.second );

    vk::CommandBufferAllocateInfo allocInfo { };
    allocInfo.setCommandPool( *VulkanAPI::GetInstance( ).GetTransferCommandPool( ) );
    allocInfo.setLevel( vk::CommandBufferLevel::ePrimary );
    allocInfo.setCommandBufferCount( 1 );

    auto  commandBuffers = VulkanAPI::GetInstance( ).getLogicalDevice( ).allocateCommandBuffersUnique( allocInfo );
    auto& commandBuffer  = commandBuffers.begin( )->get( );

    commandBuffer.begin( { vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );

    vk::BufferCopy bufferRegion;
    bufferRegion.setSize( allocation.region.vertex.second ).setDstOffset( allocation.region.vertex.first );
    commandBuffer.copyBuffer( stagingBuffer.GetBuffer( ), allocation.targetChunk->vertexBuffer, bufferRegion );

    bufferRegion.setSize( std::ceil( SCVISCC::ConvertToSecond( allocation.region.vertex.second ) ) ).setDstOffset( SCVISCC::ConvertToSecond( allocation.region.vertex.first ) ).setSrcOffset( allocation.region.vertex.second );
    commandBuffer.copyBuffer( stagingBuffer.GetBuffer( ), allocation.targetChunk->indexBuffer, bufferRegion );

    commandBuffer.end( );

    const auto& transferFamilyIndices = VulkanAPI::GetInstance( ).GetTransferFamilyIndices( );
    auto        transferQueue         = VulkanAPI::GetInstance( ).getLogicalDevice( ).getQueue( transferFamilyIndices.first, transferFamilyIndices.second );

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers( commandBuffer );
    transferQueue.submit( submitInfo, nullptr );
    transferQueue.waitIdle( );
}

template <typename VertexTy, typename IndexTy, typename SizeConverter>
void
ChunkRenderBuffers<VertexTy, IndexTy, SizeConverter>::BufferChunk::UpdateIndirectDrawBuffers( )
{
    if ( !shouldUpdateIndirectDrawBuffers ) return;

    std::lock_guard<std::mutex> lock( indirectDrawBuffersMutex );
    const auto                  newIndirectDrawBufferSize = indirectCommands.size( ) * sizeof( indirectCommands[ 0 ] );
    if ( indirectDrawBufferSize < newIndirectDrawBufferSize )
    {
        indirectDrawBufferSize = newIndirectDrawBufferSize + ( IndirectDrawBufferSizeStep - ( newIndirectDrawBufferSize % IndirectDrawBufferSizeStep ) );
        // Logger::getInstance( ).LogLine( "Creating indirectDrawBuffer with size:", indirectDrawBufferSize );

        indirectDrawBuffers.Create( indirectDrawBufferSize, vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY );

        // Logger::getInstance( ).LogLine( "Creating indirectDrawStagingBuffer with size:", indirectDrawBufferSize );
        stagingBuffer.Create( indirectDrawBufferSize, vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY );
    }

    vk::BufferCopy bufferRegion;
    bufferRegion.setSize( newIndirectDrawBufferSize );
    stagingBuffer.writeBuffer( indirectCommands.data( ), newIndirectDrawBufferSize );
    indirectDrawBuffers.CopyFromBuffer( stagingBuffer, bufferRegion, VulkanAPI::GetInstance( ) );
}

#endif   // MINECRAFT_VK_MINECRAFT_WORLD_CHUNK_CHUNKRENDERBUFFERS_IMPL_HPP