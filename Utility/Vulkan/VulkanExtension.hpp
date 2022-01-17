#pragma once

#include <Include/GlobalConfig.hpp>

#include <cstdint>
#include <vector>

class VulkanExtension
{

private:
    std::vector<const char*> m_extensions;

    void LoadRequiredExtensionsGlfw();

public:
    VulkanExtension();

    inline uint32_t
    ExtensionCount()
    {
        return m_extensions.size();
    }

    inline const char**
    ExtensionStrPtr()
    {
        return m_extensions.data();
    }

    inline bool
    isUsingValidationLayer()
    {
        return GlobalConfig::getConfigData()["vulkan"]["enable_explicit_validation_callback"].get<bool>();
    }
};