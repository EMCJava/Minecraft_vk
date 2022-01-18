#include "VulkanExtension.hpp"

#include <Include/GlobalConfig.hpp>
#include <Vulkan/GraphicAPI.hpp>

VulkanExtension::VulkanExtension()
{
    LoadRequiredExtensionsGlfw();
}

void VulkanExtension::LoadRequiredExtensionsGlfw()
{
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    m_extensions.assign(glfwExtensions, glfwExtensions + glfwExtensionCount);
    for(auto& additional_extensions : GlobalConfig::getConfigData()["vulkan_additional_extensions"]){
        m_addtional_extensions_str.push_back( additional_extensions.get<std::string>());
        m_extensions.push_back(m_addtional_extensions_str.back().c_str());
    }

    LoadRequiredExtensionsDevice();

    if (isUsingValidationLayer()) {
        m_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
}

void
VulkanExtension::LoadRequiredExtensionsDevice()
{
    auto required_extension_properties = GlobalConfig::getConfigData()[ "logical_device_extensions" ];

    m_device_extensions.clear();
    std::ranges::transform(required_extension_properties, std::back_inserter(m_device_extensions_str),
                            [](const auto& property){ return property.template get<std::string>();});
    std::ranges::transform(m_device_extensions_str, std::back_inserter(m_device_extensions),
                            [](const auto& str){ return str.c_str(); });
}
