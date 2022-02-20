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
private:
    std::unique_ptr<VulkanShader>   m_vkShader;
    vk::UniqueRenderPass            m_vkRenderPass;
    vk::UniquePipelineLayout        m_vkPipelineLayout;
    std::vector<vk::UniquePipeline> m_vkPipeline;

public:
    explicit VulkanPipeline( std::unique_ptr<VulkanShader>&& vkShader );

    template <typename VertexClass = DataType::VertexDetail, typename = std::enable_if_t<std::is_base_of_v<DataType::VertexDetail, VertexClass>>>
    void Create( float width, float height, vk::Device& device, vk::SurfaceFormatKHR format );

    [[nodiscard]] vk::Pipeline   getPipeline( ) const { return m_vkPipeline.begin( )->get( ); }
    [[nodiscard]] vk::RenderPass getRenderPass( ) const { return m_vkRenderPass.get( ); }
};

#include "VulkanPipelineCreate_Impl.hpp"

#endif   // MINECRAFT_VK_VULKAN_PIPELINE_VULKANPIPELINE_HPP
