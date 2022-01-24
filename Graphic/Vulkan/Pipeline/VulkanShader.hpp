//
// Created by loys on 18/1/2022.
//

#ifndef MINECRAFT_VK_VULKAN_PIPELINE_VULKANSHADER_HPP
#define MINECRAFT_VK_VULKAN_PIPELINE_VULKANSHADER_HPP

#include <Include/GraphicAPI.hpp>

#include <string>
#include <vector>

class VulkanShader
{
    vk::UniqueShaderModule m_vkVertex_shader_module;
    vk::UniqueShaderModule m_vkFragment_shader_module;

public:
    VulkanShader( ) = default;


    bool                   InitGLSLFile( const vk::Device& device, const std::string& vertex_file_path, const std::string& fragment_file_path );
    bool                   InitGLSLString( const vk::Device& device, const std::string& vertexShader, const std::string& fragmentShader );
    bool                   InitGLSLBinary( const vk::Device& device, const std::string& vertex_binary_path, const std::string& fragment_binary_path );
    vk::UniqueShaderModule InitGLSLCode( const vk::Device& device, const std::vector<char>& vertex_code );
    vk::UniqueShaderModule InitGLSLCode( const vk::Device& device, const std::vector<uint32_t>& vertex_code );

    friend class VulkanPipeline;
};


#endif   // MINECRAFT_VK_VULKAN_PIPELINE_VULKANSHADER_HPP
