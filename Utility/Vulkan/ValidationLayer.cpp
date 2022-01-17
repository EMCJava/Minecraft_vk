//
// Created by loys on 16/1/2022.
//

#include "ValidationLayer.hpp"

#include <Include/GlobalConfig.hpp>
#include <Utility/Logger.hpp>

#include <algorithm>

ValidationLayer::ValidationLayer()
{
    LoadLayers();
    LoadRequiredLayers();

    for ( auto& layer : m_requiredLayer )
    {

        m_isAvailable = m_isAvailable
            && std::any_of( m_availableLayers.begin(), m_availableLayers.end(),
                            [ &layer ]( const auto& layer_it ) -> bool {
                                return layer == layer_it.layerName;
                            } );
    }
}

void
ValidationLayer::LoadLayers()
{
    uint32_t layerCount;
    auto result = vk::enumerateInstanceLayerProperties( &layerCount, nullptr );
    assert( result == vk::Result::eSuccess );

    m_availableLayers.resize( layerCount );

    result = vk::enumerateInstanceLayerProperties( &layerCount, m_availableLayers.data() );
    assert( result == vk::Result::eSuccess );
}

void
ValidationLayer::LoadRequiredLayers()
{
    for ( auto& layer : GlobalConfig::getConfigData()[ "validation_layer" ] )
        m_requiredLayer.emplace_back( layer.get<std::string>() );
}

VKAPI_ATTR VkBool32 VKAPI_CALL
ValidationLayer::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData )
{
    Logger::Color log_color = Logger::Color::wReset;

    switch ( messageSeverity )
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        log_color = Logger::Color::eRed;
    }

    Logger::getInstance().LogLine( log_color, "validation layer: [",
                               vk::to_string( (vk::DebugUtilsMessageSeverityFlagBitsEXT) messageSeverity ),
                               "] -> [",
                               vk::to_string( (vk::DebugUtilsMessageTypeFlagBitsEXT) messageType ),
                               "] ->",
                               pCallbackData->pMessage);

    return VK_FALSE;
}

void
ValidationLayer::glfwErrorCallback( int code, const char* description )
{
    Logger::getInstance().LogLine( Logger::Color::eRed, "GLFW Error [", code, "]", description );
}