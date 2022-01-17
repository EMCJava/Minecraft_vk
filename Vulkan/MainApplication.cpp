//
// Created by loys on 16/1/2022.
//

#include "MainApplication.hpp"

#include "GraphicAPI.hpp"
#include <Include/GlobalConfig.hpp>

MainApplication::MainApplication()
{
    InitWindow();
    InitGraphicAPI();
}

MainApplication::~MainApplication()
{
    cleanUp();
}

void
MainApplication::run()
{

    while ( !glfwWindowShouldClose( m_window ) )
    {
        glfwPollEvents();
    }
}

void
MainApplication::InitGraphicAPI()
{
    // graphicAPIInfo();

    auto appInfo = vk::ApplicationInfo(
        "Hello Triangle",
        VK_MAKE_API_VERSION( 0, 1, 0, 0 ),
        "No Engine",
        VK_MAKE_API_VERSION( 0, 1, 0, 0 ),
        VK_API_VERSION_1_0 );

    m_vkValidationLayer = std::make_unique<ValidationLayer>();
    assert( m_vkValidationLayer->IsAvailable() );

    m_vkExtension = std::make_unique<VulkanExtension>();

    vk::InstanceCreateInfo createInfo { {}, &appInfo, m_vkValidationLayer->RequiredLayerCount(), m_vkValidationLayer->RequiredLayerStrPtr(), m_vkExtension->ExtensionCount(), m_vkExtension->ExtensionStrPtr() };

    // Validation layer
    using SeverityFlag = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using MessageFlag  = vk::DebugUtilsMessageTypeFlagBitsEXT;
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo { {},
                                                           SeverityFlag::eError | SeverityFlag::eWarning | SeverityFlag::eInfo | SeverityFlag::eVerbose,
                                                           MessageFlag::eGeneral | MessageFlag::ePerformance | MessageFlag::eValidation,
                                                           ValidationLayer::debugCallback,
                                                           {} };

    debugCreateInfo.messageSeverity ^= SeverityFlag::eInfo | SeverityFlag::eVerbose;

    // Validation layer for instance create & destroy
    if ( m_vkExtension->isUsingValidationLayer() )
    {
        createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }

    m_vkInstance = vk::createInstanceUnique( createInfo );
    m_vkDynamicDispatch.init( m_vkInstance.get(), vkGetInstanceProcAddr );

    // Validation layer
    if ( m_vkExtension->isUsingValidationLayer() )
    {
        m_vkdebugMessenger = m_vkInstance->createDebugUtilsMessengerEXT( debugCreateInfo, nullptr, m_vkDynamicDispatch );
    }

    VkSurfaceKHR rawSurface;
    if ( glfwCreateWindowSurface( m_vkInstance.get(), m_window, nullptr, &rawSurface ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create window surface!" );
    }
    m_vksurface = vk::UniqueSurfaceKHR( rawSurface, m_vkInstance.get() );

    selectPhysicalDevice();
    createLogicalDevice();
}

void
MainApplication::selectPhysicalDevice()
{
    auto vec_physical_device = m_vkInstance->enumeratePhysicalDevices();

    std::multimap<int, decltype( vec_physical_device )::value_type> device_priorities;

    for ( auto& device : vec_physical_device )
    {
        const auto device_properties = device.getProperties();
        const auto device_features   = device.getFeatures();

        Logger::getInstance().LogLine( "Found device: [", vk::to_string( device_properties.deviceType ), "]", device_properties.deviceName );

        if ( !device_features.geometryShader ) continue;   // usable

        auto device_queue_family_properties = device.getQueueFamilyProperties();
        auto first_queue_family_it          = std::find_if( device_queue_family_properties.begin(), device_queue_family_properties.end(),
                                                            []( const auto& queue_family_properties ) { return queue_family_properties.queueFlags & vk::QueueFlagBits::eGraphics; } );

        if ( first_queue_family_it == device_queue_family_properties.end() ) continue;   // unsable

        int priorities = 0;

        // Discrete GPUs have a significant performance advantage
        if ( device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
            priorities += 1000;

        // Maximum possible size of textures affects graphics quality
        priorities += device_properties.limits.maxImageDimension2D;

        device_priorities.emplace( priorities, device );
    }

    // Check if the best candidate is suitable at all
    if ( !device_priorities.empty() && device_priorities.rbegin()->first > 0 )
    {
        m_vkPhysicalDevice           = device_priorities.rbegin()->second;
        const auto device_properties = m_vkPhysicalDevice.getProperties();

        Logger::getInstance().LogLine( "Using device: [", vk::to_string( device_properties.deviceType ), "]", device_properties.deviceName );
    } else
    {
        throw std::runtime_error( "failed to find a suitable GPU!" );
    }
}

auto
MainApplication::findQueueFamilies() -> QueueFamilyIndices
{
    QueueFamilyIndices indices;

    auto queueFamilies = m_vkPhysicalDevice.getQueueFamilyProperties();

    for ( int i = 0; i < queueFamilies.size(); ++i )
    {
        if ( !indices.graphicsFamily.has_value() && queueFamilies[ i ].queueCount > 0 && queueFamilies[ i ].queueFlags & vk::QueueFlagBits::eGraphics )
        {
            indices.graphicsFamily = i;
        } else if ( !indices.presentFamily.has_value() && queueFamilies[ i ].queueCount > 0 && m_vkPhysicalDevice.getSurfaceSupportKHR( i, m_vksurface.get() ) )
        {
            indices.presentFamily = i;
        }

        if ( indices.isComplete() )
        {
            break;
        }
    }

    return indices;
}

void
MainApplication::createLogicalDevice()
{
    const auto queueFamily = findQueueFamilies();
    float queuePriority    = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.emplace_back( vk::DeviceQueueCreateFlags(), queueFamily.graphicsFamily.value(), 1, &queuePriority );
    queueCreateInfos.emplace_back( vk::DeviceQueueCreateFlags(), queueFamily.presentFamily.value(), 1, &queuePriority );

    vk::PhysicalDeviceFeatures requiredFeatures {};
    vk::DeviceCreateInfo createInfo( {}, queueCreateInfos.size(), queueCreateInfos.data(), m_vkValidationLayer->RequiredLayerCount(), m_vkValidationLayer->RequiredLayerStrPtr() );
    m_vkLogicalDevice = m_vkPhysicalDevice.createDeviceUnique( createInfo );

    m_vkGraphicQueue = m_vkLogicalDevice->getQueue( queueFamily.graphicsFamily.value(), 0 );
    m_vkPresentQueue = m_vkLogicalDevice->getQueue( queueFamily.presentFamily.value(), 0 );
}

void
MainApplication::InitWindow()
{
    glfwInit();
    glfwSetErrorCallback( ValidationLayer::glfwErrorCallback );

    m_screen_width     = GlobalConfig::getConfigData()[ "screen_width" ].get<int>();
    m_screen_height    = GlobalConfig::getConfigData()[ "screen_height" ].get<int>();
    m_window_resizable = GlobalConfig::getConfigData()[ "window_resizable" ].get<bool>();

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, m_window_resizable ? GLFW_TRUE : GLFW_FALSE );
    m_window = glfwCreateWindow( m_screen_width, m_screen_height, "Vulkan window", nullptr, nullptr );
}

void
MainApplication::cleanUp()
{
    if ( m_vkExtension->isUsingValidationLayer() )
    {
        m_vkInstance->destroyDebugUtilsMessengerEXT( m_vkdebugMessenger, {}, m_vkDynamicDispatch );
    }

    glfwDestroyWindow( m_window );
    glfwTerminate();
}

void
MainApplication::graphicAPIInfo()
{
    auto vec_extension_properties = vk::enumerateInstanceExtensionProperties();
    Logger::getInstance().Log( vec_extension_properties.size(), "extensions supported:\n" );

    for ( auto& extension : vec_extension_properties )
    {
        Logger::getInstance().Log( extension.extensionName, "\n" );
    }
}
