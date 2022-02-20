//
// Created by samsa on 2/20/2022.
//

#ifndef MINECRAFT_VK_GRAPHIC_VULKAN_PIPELINE_VULKANPIPELINECREATE_IMPL_HPP
#define MINECRAFT_VK_GRAPHIC_VULKAN_PIPELINE_VULKANPIPELINECREATE_IMPL_HPP

#include "VulkanPipeline.hpp"

template <typename VertexClass, typename>
void
VulkanPipeline::Create( float width, float height, vk::Device& device, vk::SurfaceFormatKHR format )
{
    vk::Viewport viewport { 0.0f, 0.0f, width, height, 0.0f, 1.0f };
    vk::Rect2D   scissor {
        {0, 0},
        vk::Extent2D( width, height )
    };

    vk::PipelineShaderStageCreateInfo                vert_createInfo { { }, vk::ShaderStageFlagBits::eVertex, m_vkShader->m_vkVertex_shader_module.get( ), "main" };
    vk::PipelineShaderStageCreateInfo                frag_createInfo { { }, vk::ShaderStageFlagBits::eFragment, m_vkShader->m_vkFragment_shader_module.get( ), "main" };
    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages { vert_createInfo, frag_createInfo };

    /**
     *
     *  Input
     *
     * */

    const auto vertex_binding_descriptions   = VertexClass::getBindingDescriptions( );
    const auto vertex_attribute_descriptions = VertexClass::getAttributeDescriptions( );

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo { { },
                                                             static_cast<uint32_t>( vertex_binding_descriptions.size( ) ),
                                                             vertex_binding_descriptions.data( ),
                                                             static_cast<uint32_t>( vertex_attribute_descriptions.size( ) ),
                                                             vertex_attribute_descriptions.data( ) };

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly { { },
                                                             vk::PrimitiveTopology::eTriangleList,
                                                             false };

    vk::PipelineViewportStateCreateInfo viewportState { { },
                                                        1,
                                                        &viewport,
                                                        1,
                                                        &scissor };

    /**
     *
     *
     * Rasterization
     *
     * */

    vk::PipelineRasterizationStateCreateInfo rasterizer { { },
                                                          false,
                                                          false,
                                                          vk::PolygonMode::eFill,
                                                          vk::CullModeFlagBits::eNone,
                                                          vk::FrontFace::eCounterClockwise,

                                                          vk::Bool32( false ),
                                                          { },
                                                          { },
                                                          { },

                                                          1.0f };

    vk::PipelineMultisampleStateCreateInfo multisampling { { },
                                                           vk::SampleCountFlagBits::e1,
                                                           false,
                                                           1.0f,
                                                           nullptr,
                                                           false,
                                                           false };

    /**
     *
     * Blending
     *
     * */

    vk::PipelineColorBlendAttachmentState colorBlendAttachment { false,
                                                                 vk::BlendFactor::eOne,
                                                                 vk::BlendFactor::eZero,
                                                                 vk::BlendOp::eAdd,
                                                                 vk::BlendFactor::eOne,
                                                                 vk::BlendFactor::eZero,
                                                                 vk::BlendOp::eAdd,
                                                                 vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB };

    vk::PipelineColorBlendStateCreateInfo colorBlending {
        { },
        false,
        vk::LogicOp::eCopy,
        1,
        &colorBlendAttachment,
        { 0, 0, 0, 0 }
    };

    /**
     *
     * Dynamic state
     *
     * */

    vk::DynamicState dynamicStates[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eLineWidth };

    vk::PipelineDynamicStateCreateInfo dynamicState { { }, std::size( dynamicStates ), dynamicStates };

    /**
     *
     * Pipeline layout
     *
     * */

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo { { }, 0, nullptr, 0, nullptr };
    m_vkPipelineLayout = device.createPipelineLayoutUnique( pipelineLayoutInfo );


    /**
     *
     * Render pass
     *
     * */

    vk::AttachmentDescription colorAttachment {
        { },
        format.format,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR };

    /**
     * @brief Creation of subpass
     *
     */

    vk::AttachmentReference colorAttachmentRef { 0, vk::ImageLayout::eColorAttachmentOptimal };
    vk::SubpassDescription  subpass { { },
                                     vk::PipelineBindPoint::eGraphics,
                                     0,
                                     nullptr,
                                     1,
                                     &colorAttachmentRef };

    vk::SubpassDependency subpassDependency { VK_SUBPASS_EXTERNAL, 0,
                                              vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                              vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                              vk::AccessFlags( 0 ), vk::AccessFlagBits::eColorAttachmentWrite };

    vk::RenderPassCreateInfo renderPassInfo {
        { },
        1,
        &colorAttachment,
        1,
        &subpass,
        1,
        &subpassDependency };

    m_vkRenderPass = device.createRenderPassUnique( renderPassInfo );

    vk::GraphicsPipelineCreateInfo createInfo {
        { },
        shaderStages.size( ),
        shaderStages.data( ),
        &vertexInputInfo,
        &inputAssembly,
        nullptr,
        &viewportState,
        &rasterizer,
        &multisampling,
        nullptr,
        &colorBlending,
        nullptr,
        m_vkPipelineLayout.get( ),
        m_vkRenderPass.get( ),
        0 };

    auto result = device.createGraphicsPipelinesUnique( nullptr, createInfo );
    assert( result.result == vk::Result::eSuccess );

    m_vkPipeline = std::move( result.value );
}

#endif   // MINECRAFT_VK_GRAPHIC_VULKAN_PIPELINE_VULKANPIPELINECREATE_IMPL_HPP
