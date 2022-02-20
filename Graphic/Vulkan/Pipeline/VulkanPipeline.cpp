//
// Created by loys on 18/1/2022.
//

#include "VulkanPipeline.hpp"

#include <shaderc/shaderc.hpp>

VulkanPipeline::VulkanPipeline( std::unique_ptr<VulkanShader>&& vkShader )
    : m_vkShader( std::move( vkShader ) )
{
}