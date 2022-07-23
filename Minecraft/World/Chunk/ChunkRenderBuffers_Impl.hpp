//
// Created by samsa on 7/3/2022.
//

#ifndef MINECRAFT_VK_MINECRAFT_WORLD_CHUNK_CHUNKRENDERBUFFERS_IMPL_HPP
#define MINECRAFT_VK_MINECRAFT_WORLD_CHUNK_CHUNKRENDERBUFFERS_IMPL_HPP

#include <Minecraft/util/MinecraftConstants.hpp>

#include <Utility/Logger.hpp>

#include "ChunkRenderBuffers.hpp"
#include <cmath>

namespace
{
template <class Tuple>
decltype( auto )
sum_components( Tuple const& tuple )
{
    auto sum_them = []( auto const&... e ) -> decltype( auto ) {
        return ( e + ... );
    };
    return std::apply( sum_them, tuple );
}
}   // namespace

#define ClassName( ... )                           \
    template <typename VertexTy, typename IndexTy> \
    __VA_ARGS__ ChunkRenderBuffers<VertexTy, IndexTy>

ClassName( typename ChunkRenderBuffers<VertexTy, IndexTy>::SuitableAllocation )::CreateBuffer( uint32_t vertexDataSize, uint32_t indexDataSize )
{
    std::lock_guard<std::recursive_mutex> lock( buffersMutex );

    const auto requitedSize = AlignTo<uint32_t>( vertexDataSize, sizeof( IndexTy ) ) + indexDataSize;
    if ( m_Buffers.empty( ) )
    {
        assert( requitedSize < MaxMemoryAllocation );
        GrowCapacity( );
    }

    std::vector<SuitableAllocation> suitableAllocations;
    for ( auto& bufferChunk : m_Buffers )
    {
        for ( int i = 1; i < bufferChunk.m_DataSlots.size( ); ++i )
        {
            auto prevEnd  = AlignTo<uint32_t>( bufferChunk.m_DataSlots[ i - 1 ].GetEndingPoint( ), sizeof( VertexTy ) );
            auto sizeDiff = bufferChunk.m_DataSlots[ i ].GetStartingPoint( ) - prevEnd;
            if ( sizeDiff >= requitedSize )
            {
                suitableAllocations.push_back( SuitableAllocation {
                    &bufferChunk, {prevEnd, sizeDiff}
                } );
            }
        }

        auto firstGap = MaxMemoryAllocation;
        if ( !bufferChunk.m_DataSlots.empty( ) ) firstGap = bufferChunk.m_DataSlots.front( ).GetStartingPoint( );
        if ( firstGap >= requitedSize )
        {
            suitableAllocations.push_back( SuitableAllocation {
                &bufferChunk, {0, firstGap, 0}
            } );
        }

        auto lastGap = MaxMemoryAllocation;
        if ( !bufferChunk.m_DataSlots.empty( ) ) lastGap -= AlignTo<uint32_t>( bufferChunk.m_DataSlots.back( ).GetEndingPoint( ), sizeof( VertexTy ) );
        if ( lastGap >= requitedSize )
        {
            suitableAllocations.push_back( SuitableAllocation {
                &bufferChunk, {MaxMemoryAllocation - lastGap, lastGap, 0}
            } );
        }
    }

    if ( !suitableAllocations.empty( ) )
    {
        const auto optimalBufferSize = requitedSize + ( OptimalBufferGap << 1 );
        std::sort( suitableAllocations.begin( ), suitableAllocations.end( ), [ optimalBufferSize ]( const SuitableAllocation& a, const SuitableAllocation& b ) {
            const auto aDiff = AbsDiff( a.region.vertexSize, optimalBufferSize );
            const auto bDiff = AbsDiff( b.region.vertexSize, optimalBufferSize );

            // TODO: test if this is optimal
            if ( aDiff == bDiff )
                return a.region.vertexSize < b.region.vertexSize;

            return aDiff < bDiff;
        } );

        auto& chunkUsing  = *suitableAllocations.begin( )->targetChunk;
        auto& regionUsing = suitableAllocations.begin( )->region;

        // leave space for gap
        if ( regionUsing.vertexStartingOffset != 0 && regionUsing.vertexSize > requitedSize )
        {
            const auto additionalSpace = std::min( regionUsing.vertexSize - requitedSize, optimalBufferSize << 1 );
            regionUsing.vertexSize += additionalSpace >> 1;
        }

        SingleBufferRegion newAllocation = regionUsing;
        newAllocation.SetBufferAllocation( newAllocation.vertexStartingOffset, vertexDataSize, indexDataSize );

        const auto insertIter = std::find_if( chunkUsing.m_DataSlots.begin( ), chunkUsing.m_DataSlots.end( ), [ newAllocation ]( const SingleBufferRegion& d ) { return d.vertexStartingOffset > newAllocation.vertexStartingOffset; } );
        chunkUsing.m_DataSlots.insert( insertIter, newAllocation );

        {
            std::lock_guard<std::mutex> indirectLock( chunkUsing.indirectDrawBuffersMutex );
            auto&                       newCommand = chunkUsing.indirectCommands.emplace_back( );

            newCommand.instanceCount = 1;
            newCommand.firstInstance = 0;   // chunkUsing.m_DataSlots.size( ) - 1;
            newCommand.firstIndex    = ScaleToSecond<sizeof( IndexTy ), 1>( newAllocation.indexStartingOffset );
            newCommand.indexCount    = ScaleToSecond<sizeof( IndexTy ), 1>( indexDataSize );
        }
        chunkUsing.shouldUpdateIndirectDrawBuffers = true;

        Logger::getInstance( ).LogLine( Logger::LogType::eVerbose, "New Buffer at", std::make_tuple( newAllocation.vertexStartingOffset, newAllocation.indexStartingOffset, newAllocation.vertexSize, newAllocation.indexSize ) );
        return { &chunkUsing, newAllocation };

    } else
    {
        assert( requitedSize < MaxMemoryAllocation );
        GrowCapacity( );

        return CreateBuffer( vertexDataSize, indexDataSize );
    }
}

// #define ALTER_IN_PLACE

ClassName( typename ChunkRenderBuffers<VertexTy, IndexTy>::SuitableAllocation )::AlterBuffer( const ChunkRenderBuffers::SuitableAllocation& allocation, uint32_t vertexDataSize, uint32_t indexDataSize )
{

#ifdef ALTER_IN_PLACE

    {
        std::lock_guard<std::recursive_mutex> bufferLock( buffersMutex );
        std::lock_guard<std::mutex>           indirectLock( allocation.targetChunk->indirectDrawBuffersMutex );

        const auto& bufferSize = allocation.region.GetTotalSize( );

        const auto newRequitedSize = AlignTo<uint32_t>( vertexDataSize, sizeof( IndexTy ) ) + indexDataSize;

        auto allocationIter = std::find( allocation.targetChunk->m_DataSlots.begin( ), allocation.targetChunk->m_DataSlots.end( ), allocation.region );
        assert( allocationIter != allocation.targetChunk->m_DataSlots.end( ) );

        auto oldCommandIter = std::find_if( allocation.targetChunk->indirectCommands.begin( ), allocation.targetChunk->indirectCommands.end( ), [ originalFirstIndex = ScaleToSecond<sizeof( IndexTy ), 1>( allocation.region.indexStartingOffset ) ]( const vk::DrawIndexedIndirectCommand& command ) { return command.firstIndex == originalFirstIndex; } );
        assert( oldCommandIter != allocation.targetChunk->indirectCommands.end( ) );

        if ( newRequitedSize <= bufferSize )
        {
            // we don't allocate a buffer that is smaller than our current size

            allocationIter->SetBufferAllocation( allocationIter->vertexStartingOffset, vertexDataSize, indexDataSize );
            oldCommandIter->firstIndex = ScaleToSecond<sizeof( IndexTy ), 1>( allocationIter->indexStartingOffset );
            oldCommandIter->indexCount = ScaleToSecond<sizeof( IndexTy ), 1>( indexDataSize );

            return { allocation.targetChunk, *allocationIter };
        }

        uint32_t nextBufferStarts = MaxMemoryAllocation;
        if ( allocationIter + 1 != allocation.targetChunk->m_DataSlots.end( ) )
        {
            nextBufferStarts = ( allocationIter + 1 )->GetStartingPoint( );
        }

        if ( nextBufferStarts - allocationIter->GetStartingPoint( ) >= newRequitedSize )
        {
            // there's enough space for expanding this allocation

            allocationIter->SetBufferAllocation( allocationIter->vertexStartingOffset, vertexDataSize, indexDataSize );
            oldCommandIter->firstIndex = ScaleToSecond<sizeof( IndexTy ), 1>( allocationIter->indexStartingOffset );
            oldCommandIter->indexCount = ScaleToSecond<sizeof( IndexTy ), 1>( indexDataSize );

            return { allocation.targetChunk, *allocationIter };
        }

        Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Relocating buffer allocation" );

        // else delete this allocation and find new allocation
        allocation.targetChunk->m_DataSlots.erase( allocationIter );
        allocation.targetChunk->indirectCommands.erase( oldCommandIter );
    }

#else

    {
        std::lock_guard<std::mutex> lock( m_PendingErasesLock );
        std::lock_guard<std::mutex> indirectLock( allocation.targetChunk->indirectDrawBuffersMutex );

        auto oldCommandIter = std::find_if( allocation.targetChunk->indirectCommands.begin( ), allocation.targetChunk->indirectCommands.end( ), [ originalFirstIndex = ScaleToSecond<sizeof( IndexTy ), 1>( allocation.region.indexStartingOffset ) ]( const vk::DrawIndexedIndirectCommand& command ) { return command.firstIndex == originalFirstIndex; } );
        assert( oldCommandIter != allocation.targetChunk->indirectCommands.end( ) );
        allocation.targetChunk->indirectCommands.erase( oldCommandIter );

        m_PendingErases.emplace_back( allocation, VulkanAPI::GetInstance( ).getSwapChainImagesCount( ) );
    }

#endif

    return CreateBuffer( vertexDataSize, indexDataSize );
}

ClassName( void )::GrowCapacity( )
{
    using Usage = vk::BufferUsageFlagBits;

    auto& newBuffer = m_Buffers.emplace_back( );

    vk::BufferCreateInfo    bufferInfo { { }, MaxMemoryAllocation, Usage::eVertexBuffer | Usage::eIndexBuffer | Usage::eTransferDst, vk::SharingMode::eExclusive };
    VmaAllocationCreateInfo allocInfo = { };
    allocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.flags                   = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    vmaCreateBuffer( allocator, reinterpret_cast<const VkBufferCreateInfo*>( &bufferInfo ), &allocInfo, reinterpret_cast<VkBuffer*>( &newBuffer.buffer ), &newBuffer.bufferAllocation, nullptr );
}

ClassName( )::~ChunkRenderBuffers( )
{
    Clean( );
}

ClassName( void )::CopyBuffer( ChunkRenderBuffers::SuitableAllocation allocation, void* vertexBuffer, void* indexBuffer )
{
    using Usage = vk::BufferUsageFlagBits;

    BufferMeta stagingBuffer;

    const vk::DeviceSize stagingSize = allocation.region.GetTotalSize( );
    stagingBuffer.Create( stagingSize, Usage::eVertexBuffer | Usage::eIndexBuffer | Usage::eTransferSrc );
    stagingBuffer.writeBuffer( vertexBuffer, allocation.region.vertexSize );
    stagingBuffer.writeBufferOffseted( indexBuffer, allocation.region.indexSize, allocation.region.GetOffsetDiff( ) );

    vk::CommandBufferAllocateInfo allocInfo { };
    allocInfo.setCommandPool( *VulkanAPI::GetInstance( ).GetTransferCommandPool( ) );
    allocInfo.setLevel( vk::CommandBufferLevel::ePrimary );
    allocInfo.setCommandBufferCount( 1 );

    auto  commandBuffers = VulkanAPI::GetInstance( ).getLogicalDevice( ).allocateCommandBuffersUnique( allocInfo );
    auto& commandBuffer  = commandBuffers.begin( )->get( );

    commandBuffer.begin( { vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );

    vk::BufferCopy bufferRegion;
    bufferRegion.setSize( stagingSize ).setDstOffset( allocation.region.vertexStartingOffset );
    commandBuffer.copyBuffer( stagingBuffer.GetBuffer( ), allocation.targetChunk->buffer, bufferRegion );

    commandBuffer.end( );

    const auto& transferFamilyIndices = VulkanAPI::GetInstance( ).GetTransferFamilyIndices( );
    auto        transferQueue         = VulkanAPI::GetInstance( ).getLogicalDevice( ).getQueue( transferFamilyIndices.first, transferFamilyIndices.second );

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers( commandBuffer );
    transferQueue.submit( submitInfo, nullptr );
    transferQueue.waitIdle( );
}

ClassName( void )::DeleteBuffer( const ChunkRenderBuffers::SuitableAllocation& allocation )
{

    std::lock_guard<std::recursive_mutex> bufferLock( buffersMutex );
    Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Deleting buffer", allocation );

    auto allocationIter = std::find( allocation.targetChunk->m_DataSlots.begin( ), allocation.targetChunk->m_DataSlots.end( ), allocation.region );
    assert( allocationIter != allocation.targetChunk->m_DataSlots.end( ) );

    allocation.targetChunk->m_DataSlots.erase( allocationIter );
}

ClassName( void )::BufferChunk::UpdateIndirectDrawBuffers( )
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