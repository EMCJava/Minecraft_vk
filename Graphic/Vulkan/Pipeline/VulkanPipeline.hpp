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
        std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStagesCreateInfo;
        std::vector<vk::DescriptorSet>                   vertexUniformDescriptorSetsPtr;

        // should be destruct inorder
        vk::PipelineVertexInputStateCreateInfo   vertexInputInfo;
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo;
        vk::PipelineViewportStateCreateInfo      viewportStateCreateInfo;
        vk::PipelineRasterizationStateCreateInfo rasterizerCreateInfo;
        vk::PipelineMultisampleStateCreateInfo   multisamplingCreateInfo;
        vk::PipelineColorBlendAttachmentState    colorBlendAttachmentState;
        vk::PipelineColorBlendStateCreateInfo    colorBlendingCreateInfo;
        std::vector<vk::DynamicState>            dynamicStates;
        vk::PipelineDynamicStateCreateInfo       dynamicStateCreateInfo;
        vk::PipelineLayoutCreateInfo             pipelineLayoutCreateInfo;
        vk::AttachmentDescription                colorAttachment;
        vk::AttachmentDescription                depthAttachment;
        vk::AttachmentReference                  colorAttachmentRef;
        vk::AttachmentReference                  depthAttachmentRef;
        vk::SubpassDescription                   subpass;
        vk::SubpassDependency                    subpassDependency;
        vk::RenderPassCreateInfo                 renderPassCreateInfo;
        vk::UniqueDescriptorSetLayout            vertexUniformDescriptorSetLayout;
        vk::DescriptorPoolSize                   descriptorPoolSize;
        vk::DescriptorPoolCreateInfo             descriptorPoolCreateInfo;
        vk::UniqueDescriptorPool                 descriptorPool;
        vk::PipelineDepthStencilStateCreateInfo  depthStencilCreateInfo;

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

    virtual void SetupDescriptorPool( vk::Device&, uint32_t descriptorCount );

    virtual void SetupDescriptorSet( vk::Device&, uint32_t descriptorCount );

    virtual void SetupRenderPass( vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::SurfaceFormatKHR imageFormat, vk::SurfaceFormatKHR depthFormat );

    virtual void SetupDepthStencilStage(  );

public:
    explicit VulkanPipeline( std::unique_ptr<VulkanShader>&& vkShader );

    template <typename VertexClass = DataType::VertexDetail, typename = std::enable_if_t<std::is_base_of_v<DataType::VertexDetail, VertexClass>>>
    void Create( float width, float height, uint32_t descriptorCount, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::SurfaceFormatKHR imageFormat, vk::SurfaceFormatKHR depthFormat );

    [[nodiscard]] vk::Pipeline   getPipeline( ) const { return m_vkPipeline.begin( )->get( ); }
    [[nodiscard]] vk::RenderPass getRenderPass( ) const { return m_vkRenderPass.get( ); }

    friend class VulkanAPI;
};

#include "VulkanPipelineCreate_Impl.hpp"

#endif   // MINECRAFT_VK_VULKAN_PIPELINE_VULKANPIPELINE_HPP
