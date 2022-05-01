//
// Created by loys on 18/1/2022.
//

#ifndef MINECRAFT_VK_VULKAN_PIPELINE_VULKANPIPELINE_HPP
#define MINECRAFT_VK_VULKAN_PIPELINE_VULKANPIPELINE_HPP

#include <Include/GLM.hpp>

#include "VulkanShader.hpp"

#include <memory>
#include <vector>

namespace DataType
{

struct VertexDetail {
    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions( ) { return { }; };
    static std::vector<vk::VertexInputBindingDescription>   getBindingDescriptions( ) { return { }; };
};

}   // namespace DataType

class VulkanPipeline
{
protected:
    struct PipelineCreateInfo {

        vk::Viewport viewport;
        vk::Rect2D   scissor;

        std::vector<vk::VertexInputAttributeDescription> shaderAttributeDescriptions;
        std::vector<vk::VertexInputBindingDescription>   shaderBindingDescriptions;
        std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;

        vk::PipelineVertexInputStateCreateInfo           vertexInputInfo;
        vk::PipelineInputAssemblyStateCreateInfo         inputAssembly;
        vk::PipelineViewportStateCreateInfo              viewportState;
        vk::PipelineRasterizationStateCreateInfo         rasterizer;
        vk::PipelineMultisampleStateCreateInfo           multisampling;
        vk::PipelineColorBlendAttachmentState            colorBlendAttachment;
        vk::PipelineColorBlendStateCreateInfo            colorBlending;
        std::vector<vk::DynamicState>                    dynamicStates;
        vk::PipelineDynamicStateCreateInfo               dynamicState;
        vk::PipelineLayoutCreateInfo                     pipelineLayoutInfo;
        vk::AttachmentDescription                        colorAttachment;
        vk::AttachmentReference                          colorAttachmentRef;
        vk::SubpassDescription                           subpass;
        vk::SubpassDependency                            subpassDependency;
        vk::RenderPassCreateInfo                         renderPassInfo;


        vk::GraphicsPipelineCreateInfo createInfo;
    };

    std::unique_ptr<VulkanShader>   m_vkShader;
    vk::UniqueRenderPass            m_vkRenderPass;
    vk::UniquePipelineLayout        m_vkPipelineLayout;
    std::vector<vk::UniquePipeline> m_vkPipeline;

    PipelineCreateInfo createInfo;

    virtual void SetupPipelineShaderStage( );

    template <typename VertexClass>
    void SetupInputStage( float width, float height );

    virtual void SetupRasterizationStage( );

    virtual void SetupBlendingStage( );

    virtual void SetupDynamicStage( );

    virtual void SetupPipelineLayout( vk::Device& );

    virtual void SetupRenderPass( vk::Device& device, vk::SurfaceFormatKHR format );

public:
    explicit VulkanPipeline( std::unique_ptr<VulkanShader>&& vkShader );

    template <typename VertexClass = DataType::VertexDetail, typename = std::enable_if_t<std::is_base_of_v<DataType::VertexDetail, VertexClass>>>
    void Create( float width, float height, vk::Device& device, vk::SurfaceFormatKHR format );

    [[nodiscard]] vk::Pipeline   getPipeline( ) const { return m_vkPipeline.begin( )->get( ); }
    [[nodiscard]] vk::RenderPass getRenderPass( ) const { return m_vkRenderPass.get( ); }
};

#include "VulkanPipelineCreate_Impl.hpp"

#endif   // MINECRAFT_VK_VULKAN_PIPELINE_VULKANPIPELINE_HPP
