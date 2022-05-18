//
// Created by loys on 18/1/2022.
//

#include "VulkanPipeline.hpp"

#include <shaderc/shaderc.hpp>

VulkanPipeline::VulkanPipeline( std::unique_ptr<VulkanShader>&& vkShader )
    : m_vkShader( std::move( vkShader ) )
{
}

void
VulkanPipeline::SetupPipelineShaderStage( )
{
    vk::PipelineShaderStageCreateInfo vert_createInfo { { }, vk::ShaderStageFlagBits::eVertex, m_vkShader->m_vkVertex_shader_module.get( ), "main" };
    vk::PipelineShaderStageCreateInfo frag_createInfo { { }, vk::ShaderStageFlagBits::eFragment, m_vkShader->m_vkFragment_shader_module.get( ), "main" };
    createInfo.shaderStagesCreateInfo = { vert_createInfo, frag_createInfo };
}

void
VulkanPipeline::SetupRasterizationStage( )
{
    createInfo.rasterizerCreateInfo = { { },
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

    createInfo.multisamplingCreateInfo = { { },
                                           vk::SampleCountFlagBits::e1,
                                           false,
                                           1.0f,
                                           nullptr,
                                           false,
                                           false };
}

void
VulkanPipeline::SetupBlendingStage( )
{

    createInfo.colorBlendAttachmentState = { false,
                                             vk::BlendFactor::eOne,
                                             vk::BlendFactor::eZero,
                                             vk::BlendOp::eAdd,
                                             vk::BlendFactor::eOne,
                                             vk::BlendFactor::eZero,
                                             vk::BlendOp::eAdd,
                                             vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB };

    createInfo.colorBlendingCreateInfo = {
        { },
        false,
        vk::LogicOp::eCopy,
        1,
        &createInfo.colorBlendAttachmentState,
        { 0, 0, 0, 0 }
    };
}

void
VulkanPipeline::SetupDynamicStage( )
{
    createInfo.dynamicStates = {
        vk::DynamicState::eViewport };

    createInfo.dynamicStateCreateInfo = { { }, static_cast<uint32_t>( std::size( createInfo.dynamicStates ) ), createInfo.dynamicStates.data( ) };
}


void
VulkanPipeline::SetupPipelineLayout( vk::Device& device )
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding;
    uboLayoutBinding.setBinding( 0 )
        .setDescriptorType( vk::DescriptorType::eUniformBuffer )
        .setDescriptorCount( 1 )
        .setStageFlags( vk::ShaderStageFlagBits::eVertex )
        .setPImmutableSamplers( nullptr );

    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.setBindings( uboLayoutBinding );

    createInfo.vertexUniformDescriptorSetLayout = device.createDescriptorSetLayoutUnique( layoutInfo );

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setSetLayouts( *createInfo.vertexUniformDescriptorSetLayout );
    m_vkPipelineLayout = device.createPipelineLayoutUnique( pipelineLayoutInfo );
}

void
VulkanPipeline::SetupRenderPass( vk::Device& device, vk::SurfaceFormatKHR format )
{

    createInfo.colorAttachment = {
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
     *
     * Creation of subpass
     *
     */

    createInfo.colorAttachmentRef = { 0, vk::ImageLayout::eColorAttachmentOptimal };
    createInfo.subpass            = { { },
                                      vk::PipelineBindPoint::eGraphics,
                                      0,
                                      nullptr,
                                      1,
                                      &createInfo.colorAttachmentRef };

    createInfo.subpassDependency = { VK_SUBPASS_EXTERNAL, 0,
                                     vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                     vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                     vk::AccessFlags( 0 ), vk::AccessFlagBits::eColorAttachmentWrite };

    createInfo.renderPassCreateInfo = {
        { },
        1,
        &createInfo.colorAttachment,
        1,
        &createInfo.subpass,
        1,
        &createInfo.subpassDependency };

    m_vkRenderPass = device.createRenderPassUnique( createInfo.renderPassCreateInfo );
}

void
VulkanPipeline::SetupDescriptorPool( vk::Device& device, uint32_t descriptorCount )
{
    createInfo.descriptorPoolSize.setType( vk::DescriptorType::eUniformBuffer );
    createInfo.descriptorPoolSize.setDescriptorCount( descriptorCount );

    createInfo.descriptorPoolCreateInfo.setPoolSizes( createInfo.descriptorPoolSize );
    createInfo.descriptorPoolCreateInfo.setMaxSets( descriptorCount );

    createInfo.descriptorPool = device.createDescriptorPoolUnique( createInfo.descriptorPoolCreateInfo );
}

void
VulkanPipeline::SetupDescriptorSet( vk::Device& device, uint32_t descriptorCount )
{
    // TODO: save?
    std::vector<vk::DescriptorSetLayout> layouts( descriptorCount, *createInfo.vertexUniformDescriptorSetLayout );

    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.setDescriptorPool( *createInfo.descriptorPool )
        .setDescriptorSetCount( descriptorCount )
        .setSetLayouts( layouts );

    createInfo.vertexUniformDescriptorSetsPtr.resize( descriptorCount );
    createInfo.vertexUniformDescriptorSetsPtr = device.allocateDescriptorSets( allocInfo );
}
