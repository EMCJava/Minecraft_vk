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
    std::vector<std::string> m_addtional_extensions_str;

    void LoadRequiredExtensionsGlfw( );
    void LoadRequiredExtensionsDevice( );

public:
    VulkanExtension( );

    inline size_t
    VulkanExtensionCount( )
    {
        return m_extensions.size( );
    }

    inline const char**
    VulkanExtensionStrPtr( )
    {
        return m_extensions.data( );
    }

    inline size_t
    DeviceExtensionCount( )
    {
        return m_device_extensions.size( );
    }

    inline const char**
    DeviceExtensionStrPtr( )
    {
        return m_device_extensions.data( );
    }

    inline bool
    isUsingValidationLayer( )
    {
        return GlobalConfig::getConfigData( )[ "vulkan" ][ "enable_explicit_validation_callback" ].get<bool>( );
    }
};