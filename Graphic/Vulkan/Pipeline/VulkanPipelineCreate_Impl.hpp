//
// Created by samsa on 2/20/2022.
//

#ifndef MINECRAFT_VK_GRAPHIC_VULKAN_PIPELINE_VULKANPIPELINECREATE_IMPL_HPP
#define MINECRAFT_VK_GRAPHIC_VULKAN_PIPELINE_VULKANPIPELINECREATE_IMPL_HPP

#include "VulkanPipeline.hpp"


template <typename VertexClass>
void
VulkanPipeline::SetupInputStage( float width, float height )
{
    createInfo.viewport = vk::Viewport{ 0.0f, 0.0f, width, height, 0.0f, 1.0f };
    createInfo.scissor  = {
         {0, 0},
         vk::Extent2D( static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) )
    };

    createInfo.shaderBindingDescriptions   = VertexClass::getBindingDescriptions( );
    createInfo.shaderAttributeDescriptions = VertexClass::getAttributeDescriptions( );

    createInfo.vertexInputInfo = { { },
                                   static_cast<uint32_t>( createInfo.shaderBindingDescriptions.size( ) ),
                                   createInfo.shaderBindingDescriptions.data( ),
                                   static_cast<uint32_t>( createInfo.shaderAttributeDescriptions.size( ) ),
                                   createInfo.shaderAttributeDescriptions.data( ) };

    createInfo.inputAssembly = { { },
                                 vk::PrimitiveTopology::eTriangleList,
                                 false };

    createInfo.viewportState = { { },
                                 1,
                                 &createInfo.viewport,
                                 1,
                                 &createInfo.scissor };
}


template <typename VertexClass, typename>
void
VulkanPipeline::Create( float width, float height, vk::Device& device, vk::SurfaceFormatKHR format )
{
    SetupPipelineShaderStage( );

    /**
     *
     *  Input
     *
     * */

    SetupInputStage<VertexClass>( width, height );

    /**
     *
     *
     * Rasterization
     *
     * */

    SetupRasterizationStage( );

    /**
     *
     * Blending
     *
     * */

    SetupBlendingStage( );

    /**
     *
     * Dynamic state
     *
     * */

    SetupDynamicStage( );

    /**
     *
     * Pipeline layout
     *
     * */

    SetupPipelineLayout( device );

    /**
     *
     * Render pass
     *
     * */
    SetupRenderPass( device, format );

    createInfo.createInfo.setStageCount( createInfo.shaderStages.size( ) );
    createInfo.createInfo.setStages( createInfo.shaderStages );
    createInfo.createInfo.setPVertexInputState( &createInfo.vertexInputInfo );
    createInfo.createInfo.setPInputAssemblyState( &createInfo.inputAssembly );
    createInfo.createInfo.setPViewportState( &createInfo.viewportState );
    createInfo.createInfo.setPRasterizationState( &createInfo.rasterizer );
    createInfo.createInfo.setPMultisampleState( &createInfo.multisampling );
    createInfo.createInfo.setPColorBlendState( &createInfo.colorBlending );
    createInfo.createInfo.setLayout( m_vkPipelineLayout.get( ) );
    createInfo.createInfo.setRenderPass( m_vkRenderPass.get( ) );
    createInfo.createInfo.setSubpass( 0 );

    createInfo.createInfo.setPTessellationState( nullptr );
    createInfo.createInfo.setPDepthStencilState( nullptr );
    createInfo.createInfo.setPDynamicState( nullptr );

    auto result = device.createGraphicsPipelinesUnique( nullptr, createInfo.createInfo );
    assert( result.result == vk::Result::eSuccess );

    m_vkPipeline = std::move( result.value );
}

#endif   // MINECRAFT_VK_GRAPHIC_VULKAN_PIPELINE_VULKANPIPELINECREATE_IMPL_HPP
