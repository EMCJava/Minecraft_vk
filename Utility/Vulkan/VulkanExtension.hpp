#pragma once

#include <Include/GlobalConfig.hpp>

#include <cstdint>
#include <vector>

class VulkanExtension
{

private:
    std::vector<const char*> m_extensions;
    std::vector<std::string> m_device_extensions_str;
    std::vector<const char*> m_device_extensions;

    void LoadRequiredExtensionsGlfw();
    void LoadRequiredExtensionsDevice();

public:
    VulkanExtension();

    inline uint32_t
    VulkanExtensionCount()
    {
        return m_extensions.size();
    }

    inline const char**
    VulkanExtensionStrPtr()
    {
        return m_extensions.data();
    }

    inline uint32_t
    DeviceExtensionCount()
    {
        return m_device_extensions.size();
    }

    inline const char**
    DeviceExtensionStrPtr()
    {
        return m_device_extensions.data();
    }

    inline bool
    isUsingValidationLayer()
    {
        return GlobalConfig::getConfigData()["vulkan"]["enable_explicit_validation_callback"].get<bool>();
    }
};