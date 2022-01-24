//
// Created by loys on 18/1/2022.
//

#ifndef MINECRAFT_VK_VULKAN_PIPELINE_VULKANPIPELINE_HPP
#define MINECRAFT_VK_VULKAN_PIPELINE_VULKANPIPELINE_HPP

#include "VulkanShader.hpp"

#include <memory>

class VulkanPipeline
{
private:
    std::unique_ptr<VulkanShader>   m_vkShader;
    vk::UniqueRenderPass            m_vkRenderPass;
    vk::UniquePipelineLayout        m_vkPipelineLayout;
    std::vector<vk::UniquePipeline> m_vkPipeline;

public:
    explicit VulkanPipeline( std::unique_ptr<VulkanShader>&& vkShader );

    void                         Create( float width, float height, vk::Device& device, vk::SurfaceFormatKHR format );

    [[nodiscard]] vk::Pipeline   getPipeline( ) const { return m_vkPipeline.begin( )->get( ); }
    [[nodiscard]] vk::RenderPass getRenderPass( ) const { return m_vkRenderPass.get( ); }
};


#endif   // MINECRAFT_VK_VULKAN_PIPELINE_VULKANPIPELINE_HPP
