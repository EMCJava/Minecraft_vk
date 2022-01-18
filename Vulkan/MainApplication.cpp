//
// Created by loys on 16/1/2022.
//

#include "MainApplication.hpp"

#include "GraphicAPI.hpp"
#include <Include/GlobalConfig.hpp>

#include <limits>
#include <unordered_set>

MainApplication::MainApplication( )
{
    InitWindow( );
    InitGraphicAPI( );

    Logger::getInstance( ).LogLine( "Finished Initializing" );
}

MainApplication::~MainApplication( )
{
    cleanUp( );
}

void
MainApplication::run( )
{

    while ( !glfwWindowShouldClose( m_window ) )
    {
        glfwPollEvents( );
        m_vkLogicalDevice->waitIdle( );
    }
}

void
MainApplication::InitGraphicAPI( )
{
    // graphicAPIInfo();

    auto appInfo = vk::ApplicationInfo(
        "Hello Triangle",
        VK_MAKE_API_VERSION( 0, 1, 0, 0 ),
        "No Engine",
        VK_MAKE_API_VERSION( 0, 1, 0, 0 ),
        VK_API_VERSION_1_0 );

    m_vkValidationLayer = std::make_unique<ValidationLayer>( );
    assert( m_vkValidationLayer->IsAvailable( ) );

    m_vkExtension = std::make_unique<VulkanExtension>( );

    vk::InstanceCreateInfo createInfo { { }, &appInfo, m_vkValidationLayer->RequiredLayerCount( ), m_vkValidationLayer->RequiredLayerStrPtr( ), m_vkExtension->VulkanExtensionCount( ), m_vkExtension->VulkanExtensionStrPtr( ) };

    // Validation layer
    using SeverityFlag = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using MessageFlag  = vk::DebugUtilsMessageTypeFlagBitsEXT;
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo { { },
                                                           SeverityFlag::eError | SeverityFlag::eWarning | SeverityFlag::eInfo | SeverityFlag::eVerbose,
                                                           MessageFlag::eGeneral | MessageFlag::ePerformance | MessageFlag::eValidation,
                                                           ValidationLayer::debugCallback,
                                                           {} };

    debugCreateInfo.messageSeverity ^= SeverityFlag::eInfo | SeverityFlag::eVerbose;

    // Validation layer for instance create & destroy
    if ( m_vkExtension->isUsingValidationLayer( ) )
    {
        createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }

    m_vkInstance = vk::createInstanceUnique( createInfo );
    m_vkDynamicDispatch.init( m_vkInstance.get( ), vkGetInstanceProcAddr );

    // Validation layer
    if ( m_vkExtension->isUsingValidationLayer( ) )
    {
        m_vkDebugMessenger = m_vkInstance->createDebugUtilsMessengerEXT( debugCreateInfo, nullptr, m_vkDynamicDispatch );
    }

    VkSurfaceKHR rawSurface;
    if ( glfwCreateWindowSurface( m_vkInstance.get( ), m_window, nullptr, &rawSurface ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create window surface!" );
    }
    m_vkSurface = vk::UniqueSurfaceKHR( rawSurface, m_vkInstance.get( ) );

    selectPhysicalDevice( );
    createLogicalDevice( );
    createSwapChain( );
}

void
MainApplication::selectPhysicalDevice( )
{
    auto                                                            vec_physical_device = m_vkInstance->enumeratePhysicalDevices( );

    std::multimap<int, decltype( vec_physical_device )::value_type> device_priorities;

    for ( auto& device : vec_physical_device )
    {
        const auto device_properties = device.getProperties( );
        const auto device_features   = device.getFeatures( );

        Logger::getInstance( ).LogLine( "Found device: [", vk::to_string( device_properties.deviceType ), "]", device_properties.deviceName );

        if ( !device_features.geometryShader ) continue;   // usable
        if ( !checkDeviceExtensionSupport( device ) ) continue;
        if ( !setSwapChainSupportDetails( device ) ) continue;

        const auto device_queue_family_properties = device.getQueueFamilyProperties( );
        const auto first_queue_family_it          = std::find_if( device_queue_family_properties.begin( ), device_queue_family_properties.end( ),
                                                                  []( const auto& queue_family_properties ) { return queue_family_properties.queueFlags & vk::QueueFlagBits::eGraphics; } );

        if ( first_queue_family_it == device_queue_family_properties.end( ) ) continue;   // unsable

        uint32_t priorities = 0;

        // Discrete GPUs have a significant performance advantage
        if ( device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
            priorities += 1000;

        // Maximum possible size of textures affects graphics quality
        priorities += device_properties.limits.maxImageDimension2D;

        device_priorities.emplace( priorities, device );
    }

    // Check if the best candidate is suitable at all
    if ( !device_priorities.empty( ) && device_priorities.rbegin( )->first > 0 )
    {
        m_vkPhysicalDevice           = device_priorities.rbegin( )->second;
        const auto device_properties = m_vkPhysicalDevice.getProperties( );

        Logger::getInstance( ).LogLine( "Using device: [", vk::to_string( device_properties.deviceType ), "]", device_properties.deviceName );
    } else
    {
        throw std::runtime_error( "failed to find a suitable GPU!" );
    }
}

bool
MainApplication::checkDeviceExtensionSupport( const vk::PhysicalDevice& device )
{

    const auto                      vec_extension_properties = device.enumerateDeviceExtensionProperties( );

    std::unordered_set<std::string> extension_properties_name_set;
    for ( const auto& extension_properties : vec_extension_properties )
        extension_properties_name_set.insert( extension_properties.extensionName );

    auto required_extension_properties = GlobalConfig::getConfigData( )[ "logical_device_extensions" ];
    return std::ranges::all_of( required_extension_properties.begin( ), required_extension_properties.end( ),
                                [ &extension_properties_name_set ]( const auto& property ) { return extension_properties_name_set.contains( property.template get<std::string>( ) ); } );
}

bool
MainApplication::setSwapChainSupportDetails( const vk::PhysicalDevice& device )
{
    m_vkSwap_chain_detail.capabilities = device.getSurfaceCapabilitiesKHR( m_vkSurface.get( ) );
    m_vkSwap_chain_detail.presentModes = device.getSurfacePresentModesKHR( m_vkSurface.get( ) );
    m_vkSwap_chain_detail.formats      = device.getSurfaceFormatsKHR( m_vkSurface.get( ) );

    {
        auto best_format_it = std::find_if( m_vkSwap_chain_detail.formats.begin( ), m_vkSwap_chain_detail.formats.end( ),
                                            []( const auto& format ) {
                                                return format.format == vk::Format::eB8G8R8A8Srgb
                                                    && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
                                            } );

        if ( best_format_it != m_vkSwap_chain_detail.formats.begin( )
             && best_format_it != m_vkSwap_chain_detail.formats.end( ) )
            std::iter_swap( best_format_it, m_vkSwap_chain_detail.formats.begin( ) );
    }

    {

        static constexpr vk::PresentModeKHR                              FALLBACK_PRESENT_MODE = vk::PresentModeKHR::eFifo;
        static const std::unordered_map<std::string, vk::PresentModeKHR> present_mode_mapping {
            {              "Immediate",               vk::PresentModeKHR::eImmediate},
            {                "Mailbox",                 vk::PresentModeKHR::eMailbox},
            {                   "Fifo",                    vk::PresentModeKHR::eFifo},
            {            "FifoRelaxed",             vk::PresentModeKHR::eFifoRelaxed},
            {    "SharedDemandRefresh",     vk::PresentModeKHR::eSharedDemandRefresh},
            {"SharedContinuousRefresh", vk::PresentModeKHR::eSharedContinuousRefresh}
        };

        vk::PresentModeKHR preferred_present_mode;
        std::string        preferred_present_mode_str;

        {
            // Get user config
            GlobalConfig::getConfigData( )[ "vulkan" ][ "presentation_mode" ].get_to( preferred_present_mode_str );
            if ( !present_mode_mapping.contains( preferred_present_mode_str ) )
                GlobalConfig::getConfigData( )[ "vulkan" ][ "fallback_presentation_mode" ].get_to( preferred_present_mode_str );
            if ( !present_mode_mapping.contains( preferred_present_mode_str ) )
                preferred_present_mode_str = "Fifo";
        }

        preferred_present_mode = present_mode_mapping.at( preferred_present_mode_str );
        auto best_present_it   = std::find_if( m_vkSwap_chain_detail.presentModes.begin( ), m_vkSwap_chain_detail.presentModes.end( ),
                                               [ preferred_present_mode ]( const auto& present ) { return present == preferred_present_mode; } );

        if ( best_present_it == m_vkSwap_chain_detail.presentModes.end( ) )
            best_present_it = std::find_if( m_vkSwap_chain_detail.presentModes.begin( ), m_vkSwap_chain_detail.presentModes.end( ),
                                            []( const auto& present ) { return present == FALLBACK_PRESENT_MODE; } );

        if ( best_present_it != m_vkSwap_chain_detail.presentModes.begin( )
             && best_present_it != m_vkSwap_chain_detail.presentModes.end( ) )
            std::iter_swap( best_present_it, m_vkSwap_chain_detail.presentModes.begin( ) );
    }

    return m_vkSwap_chain_detail.isComplete( );
}

void
MainApplication::setQueueFamiliesIndex( )
{

    m_vkQueue_family_indices.graphicsFamily.reset( );
    m_vkQueue_family_indices.presentFamily.reset( );

    auto queueFamilies = m_vkPhysicalDevice.getQueueFamilyProperties( );

    for ( int i = 0; i < queueFamilies.size( ); ++i )
    {
        if ( queueFamilies[ i ].queueCount > 0 && queueFamilies[ i ].queueFlags & vk::QueueFlagBits::eGraphics )
        {
            m_vkQueue_family_indices.graphicsFamily = i;
        }

        if ( queueFamilies[ i ].queueCount > 0 && m_vkPhysicalDevice.getSurfaceSupportKHR( i, m_vkSurface.get( ) ) )
        {
            m_vkQueue_family_indices.presentFamily = i;
        }

        if ( m_vkQueue_family_indices.isComplete( ) )
        {
            break;
        }
    }
}

void
MainApplication::createLogicalDevice( )
{
    setQueueFamiliesIndex( );

    std::unordered_set<uint32_t>           queue_index_set { m_vkQueue_family_indices.graphicsFamily.value( ), m_vkQueue_family_indices.presentFamily.value( ) };
    float                                  queuePriority = 1.0f;

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::ranges::transform( queue_index_set, std::back_inserter( queueCreateInfos ),
                            [ queuePriority = &queuePriority ]( uint32_t index ) {
                                return vk::DeviceQueueCreateInfo { vk::DeviceQueueCreateFlags( ), index, 1, queuePriority };
                            } );

    vk::PhysicalDeviceFeatures requiredFeatures { };
    vk::DeviceCreateInfo       createInfo( { },
                                           queueCreateInfos.size( ), queueCreateInfos.data( ),
                                           m_vkValidationLayer->RequiredLayerCount( ), m_vkValidationLayer->RequiredLayerStrPtr( ),
                                           m_vkExtension->DeviceExtensionCount( ), m_vkExtension->DeviceExtensionStrPtr( ) );
    m_vkLogicalDevice = m_vkPhysicalDevice.createDeviceUnique( createInfo );

    m_vkGraphicQueue  = m_vkLogicalDevice->getQueue( m_vkQueue_family_indices.graphicsFamily.value( ), 0 );
    m_vkPresentQueue  = m_vkLogicalDevice->getQueue( m_vkQueue_family_indices.presentFamily.value( ), 0 );
}

void
MainApplication::InitWindow( )
{
    glfwInit( );
    glfwSetErrorCallback( ValidationLayer::glfwErrorCallback );

    m_screen_width     = GlobalConfig::getConfigData( )[ "screen_width" ].get<int>( );
    m_screen_height    = GlobalConfig::getConfigData( )[ "screen_height" ].get<int>( );
    m_window_resizable = GlobalConfig::getConfigData( )[ "window_resizable" ].get<bool>( );

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, m_window_resizable ? GLFW_TRUE : GLFW_FALSE );
    m_window = glfwCreateWindow( m_screen_width, m_screen_height, "Vulkan window", nullptr, nullptr );
}

void
MainApplication::cleanUp( )
{
    if ( m_vkExtension->isUsingValidationLayer( ) )
    {
        m_vkInstance->destroyDebugUtilsMessengerEXT( m_vkDebugMessenger, { }, m_vkDynamicDispatch );
    }

    glfwDestroyWindow( m_window );
    glfwTerminate( );
}

void
MainApplication::graphicAPIInfo( )
{
    auto vec_extension_properties = vk::enumerateInstanceExtensionProperties( );
    Logger::getInstance( ).Log( vec_extension_properties.size( ), "extensions supported:\n" );

    for ( auto& extension : vec_extension_properties )
    {
        Logger::getInstance( ).Log( extension.extensionName, "\n" );
    }
}

void
MainApplication::createSwapChain( )
{
    // index 0 is the best we set
    vk::SurfaceFormatKHR surfaceFormat = m_vkSwap_chain_detail.formats[ 0 ];
    vk::PresentModeKHR   presentMode   = m_vkSwap_chain_detail.presentModes[ 0 ];

    VkExtent2D           extent        = m_vkSwap_chain_detail.getMaxSwapExtent( m_window );

    uint32_t             imageCount    = m_vkSwap_chain_detail.capabilities.minImageCount + 1;
    assert( imageCount <= m_vkSwap_chain_detail.capabilities.maxImageCount );

    vk::SwapchainCreateInfoKHR createInfo { { },
                                            m_vkSurface.get( ),
                                            imageCount,
                                            surfaceFormat.format,
                                            surfaceFormat.colorSpace,
                                            extent,
                                            1,
                                            vk::ImageUsageFlagBits::eColorAttachment,
                                            { },
                                            { },
                                            { },
                                            m_vkSwap_chain_detail.capabilities.currentTransform,
                                            vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                            presentMode,
                                            vk::Bool32( VK_TRUE ),
                                            m_vkSwap_chain.get( ) };

    uint32_t                   queueFamilyIndices[] = { m_vkQueue_family_indices.graphicsFamily.value( ),
                                      m_vkQueue_family_indices.presentFamily.value( ) };

    if ( m_vkQueue_family_indices.graphicsFamily != m_vkQueue_family_indices.presentFamily )
    {
        createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = std::size( queueFamilyIndices );
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    } else
    {
        createInfo.imageSharingMode      = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;         // Optional
        createInfo.pQueueFamilyIndices   = nullptr;   // Optional
    }

    m_vkSwap_chain        = m_vkLogicalDevice->createSwapchainKHRUnique( createInfo );
    m_vkSwap_chain_images = m_vkLogicalDevice->getSwapchainImagesKHR( m_vkSwap_chain.get( ) );

    m_vkSwap_chain_image_views.clear( );
    std::ranges::transform( m_vkSwap_chain_images, std::back_inserter( m_vkSwap_chain_image_views ),
                            [ this, &surfaceFormat ]( const vk::Image& image ) {
                                vk::ImageViewCreateInfo createInfo {
                                    { },
                                    image, vk::ImageViewType::e2D, surfaceFormat.format, vk::ComponentMapping { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity },
                                    vk::ImageSubresourceRange { vk::ImageAspectFlagBits::eColor,                               0,                               1,                               0, 1 }
                                };
                                return std::move( m_vkLogicalDevice->createImageViewUnique( createInfo ) );
                            } );
}

vk::Extent2D
MainApplication::SwapChainSupportDetails::getMaxSwapExtent( GLFWwindow* window ) const
{
    if ( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max( ) )
    {
        return capabilities.currentExtent;
    } else
    {
        int width, height;
        glfwGetFramebufferSize( window, &width, &height );

        VkExtent2D actualExtent = {
            static_cast<uint32_t>( width ),
            static_cast<uint32_t>( height ) };

        actualExtent.width  = std::clamp( actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
        actualExtent.height = std::clamp( actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

        return actualExtent;
    }
}
