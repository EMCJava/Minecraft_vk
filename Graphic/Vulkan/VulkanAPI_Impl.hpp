//
// Created by loys on 5/2/2022.
//

#ifndef MINECRAFT_VK_GRAPHIC_VULKAN_VULKANAPI_IMPL_HPP
#define MINECRAFT_VK_GRAPHIC_VULKAN_VULKANAPI_IMPL_HPP

#include "VulkanAPI.hpp"

template <bool doRender>
void
VulkanAPI::presentFrame( uint32_t index )
{
    if ( index == (uint32_t) -1 )
    {
        index = m_sync_index;
    }

    /**
     *
     * Sync in renderer
     *
     * */
    auto wait_fence_result = m_vkLogicalDevice->waitForFences( m_vkRender_fence_syncs[ m_sync_index ].get( ), true, std::numeric_limits<uint64_t>::max( ) );
    assert( wait_fence_result == vk::Result::eSuccess );

    /**
     *
     * Sync in swap chain image
     * In-case too many synced render on the same image
     *
     * [  1  ] [  2  ] [  3  ] [  1  ] [  2  ] [  3  ]
     *  sync                    index
     *
     * */
    if ( m_vkSwap_chain_image_fence_syncs[ index ] )
    {
        wait_fence_result = m_vkLogicalDevice->waitForFences( m_vkSwap_chain_image_fence_syncs[ m_sync_index ], true, std::numeric_limits<uint64_t>::max( ) );
        assert( wait_fence_result == vk::Result::eSuccess );
    }

    if constexpr ( doRender )
    {
        cycleGraphicCommandBuffers( index );
    }

    m_vkSwap_chain_image_fence_syncs[ index ] = m_vkRender_fence_syncs[ m_sync_index ].get( );

    /**
     *
     * Reset fence to unsignaled state, and start rendering on that index
     *
     * */
    m_vkLogicalDevice->resetFences( m_vkRender_fence_syncs[ m_sync_index ].get( ) );

    /**
     *
     * Submit
     *
     * */
    vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo         submitInfo {
        1, &m_vkImage_acquire_syncs[ m_sync_index ].get( ), &waitStageMask,
        1, &m_vkGraphicCommandBuffers[ index ],
        1, &m_vkRender_syncs[ m_sync_index ].get( ) };

    m_vkGraphicQueue.submit( submitInfo, m_vkRender_fence_syncs[ m_sync_index ].get( ) );

    /**
     *
     * Present
     *
     * */
    vk::PresentInfoKHR presentInfo { 1, &m_vkRender_syncs[ m_sync_index ].get( ),
                                     1, &m_vkSwap_chain.get( ), &index };


    try
    {
        const auto present_result = m_vkPresentQueue.presentKHR( presentInfo );
        if ( present_result == vk::Result::eSuboptimalKHR ) adeptSwapChainChange( );
        assert( present_result == vk::Result::eSuccess );
    }
    catch ( vk::OutOfDateKHRError& err )
    {
        m_swap_chain_not_valid.test_and_set( );
    }

    m_sync_index = ( m_sync_index + 1 ) % m_sync_count;
}

#endif   // MINECRAFT_VK_GRAPHIC_VULKAN_VULKANAPI_IMPL_HPP
