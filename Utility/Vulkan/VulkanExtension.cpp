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

    if (isUsingValidationLayer()) {
        m_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
}