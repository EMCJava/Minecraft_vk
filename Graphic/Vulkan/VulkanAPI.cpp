//
// Created by loys on 1/24/2022.
//

#include "VulkanAPI.hpp"

#include <Utility/Logger.hpp>
#include <Utility/Vulkan/VulkanExtension.hpp>

#include <filesystem>
#include <unordered_set>

namespace
{

vk::DebugUtilsMessengerCreateInfoEXT
getDebugUtilsMessengerCreateInfo( )
{


    using SeverityFlag = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using MessageFlag  = vk::DebugUtilsMessageTypeFlagBitsEXT;
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo { { },
                                                           SeverityFlag::eError | SeverityFlag::eWarning | SeverityFlag::eInfo | SeverityFlag::eVerbose,
                                                           MessageFlag::eGeneral | MessageFlag::ePerformance | MessageFlag::eValidation,
                                                           ValidationLayer::debugCallback,
                                                           {} };

    debugCreateInfo.messageSeverity ^= SeverityFlag::eInfo | SeverityFlag::eVerbose;

    return debugCreateInfo;
}
}   // namespace

vk::Extent2D
VulkanAPI::SwapChainSupportDetails::getMaxSwapExtent( GLFWwindow* window ) const
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

VulkanAPI::VulkanAPI( GLFWwindow* windows )
    : m_window( windows )
{
}

void
VulkanAPI::setupAPI( const std::string& applicationName )
{
    vk::ApplicationInfo appInfo;
    appInfo.setPApplicationName( applicationName.c_str( ) );
    appInfo.setApplicationVersion( VK_MAKE_API_VERSION( 0, 1, 0, 0 ) );
    appInfo.setPEngineName( "No Engine" );
    appInfo.setEngineVersion( VK_MAKE_API_VERSION( 0, 1, 0, 0 ) );
    appInfo.setApiVersion( VK_API_VERSION_1_0 );

    setupValidationLayer( );
    setupExtensions( );

    /**
     *
     * Create Instance
     *
     * */
    vk::InstanceCreateInfo createInfo;
    createInfo.setPApplicationInfo( &appInfo );
    createInfo.setEnabledLayerCount( (uint32_t) m_vkValidationLayer->RequiredLayerCount( ) );
    createInfo.setPpEnabledLayerNames( m_vkValidationLayer->RequiredLayerStrPtr( ) );
    createInfo.setEnabledExtensionCount( (uint32_t) m_vkExtension->VulkanExtensionCount( ) );
    createInfo.setPpEnabledExtensionNames( m_vkExtension->VulkanExtensionStrPtr( ) );

    // Validation layer for instance create & destroy
    const auto debugCreateInfo = ::getDebugUtilsMessengerCreateInfo( );
    if ( m_vkExtension->isUsingValidationLayer( ) ) createInfo.pNext = &debugCreateInfo;

    m_vkInstance = vk::createInstanceUnique( createInfo );

    /**
     *
     * For special function load
     *
     * */
    setupDynamicDispatch( );

    /**
     *
     * Validation layers for debug
     *
     * */
    if ( m_vkExtension->isUsingValidationLayer( ) ) setupDebugManager( debugCreateInfo );

    /**
     *
     * Setup window display
     *
     * */
    setupSurface( );

    /**
     *
     * Setup graphic card
     *
     * */
    selectPhysicalDevice( );
    setupLogicalDevice( );

    /*
     *
     * Setup Vulkan memory allocator
     *
     * */
    setupVulkanMemoryAllocator( );

    /**
     *
     * Setup render detail
     *
     * */
    setupSwapChain( );
    setupPipeline( );

    /**
     *
     * Graphic command setup
     *
     * */
    setupGraphicCommand( );

    /**
     *
     * Synchronization for rendering
     *
     * */
    setupSyncs( );
}

void
VulkanAPI::setupValidationLayer( )
{
    m_vkValidationLayer = std::make_unique<ValidationLayer>( );
    assert( m_vkValidationLayer->IsAvailable( ) );
}

void
VulkanAPI::setupExtensions( )
{
    m_vkExtension = std::make_unique<VulkanExtension>( );
}

void
VulkanAPI::setupDynamicDispatch( )
{
    m_vkDynamicDispatch.init( m_vkInstance.get( ), vkGetInstanceProcAddr );
}

void
VulkanAPI::setupDebugManager( const vk::DebugUtilsMessengerCreateInfoEXT& debugCreateInfo )
{
    m_vkDebugMessenger = m_vkInstance->createDebugUtilsMessengerEXTUnique( debugCreateInfo, nullptr, m_vkDynamicDispatch );
}

void
VulkanAPI::setupSurface( )
{
    VkSurfaceKHR rawSurface;
    if ( glfwCreateWindowSurface( m_vkInstance.get( ), m_window, nullptr, &rawSurface ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create window surface!" );
    }
    m_vkSurface = vk::UniqueSurfaceKHR( rawSurface, m_vkInstance.get( ) );
}

void
VulkanAPI::selectPhysicalDevice( )
{

    auto                                                            vec_physical_device = m_vkInstance->enumeratePhysicalDevices( );
    std::multimap<int, decltype( vec_physical_device )::value_type> device_priorities;

    for ( auto& device : vec_physical_device )
    {
        const auto device_properties = device.getProperties( );
        Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Found device: [", vk::to_string( device_properties.deviceType ), "]", device_properties.deviceName );

        /**
         *
         * Discard
         *
         * */
        if ( !isDeviceUsable( device ) ) continue;
        if ( std::ranges::none_of( device.getQueueFamilyProperties( ),
                                   []( const auto& queue_family_properties ) { return static_cast<bool>( queue_family_properties.queueFlags & vk::QueueFlagBits::eGraphics ); } ) ) continue;

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
        m_vkPhysicalDeviceProperties = m_vkPhysicalDevice.getProperties( );

        setSwapChainSupportDetails( m_vkPhysicalDevice );

        Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Using device: [", vk::to_string( m_vkPhysicalDeviceProperties.deviceType ), "]", m_vkPhysicalDeviceProperties.deviceName );
    } else
    {
        throw std::runtime_error( "failed to find a suitable GPU!" );
    }
}

bool
VulkanAPI::isDeviceSupportAllExtensions( const vk::PhysicalDevice& device )
{

    const auto vec_extension_properties = device.enumerateDeviceExtensionProperties( );

    std::unordered_set<std::string> extension_properties_name_set;
    for ( const auto& extension_properties : vec_extension_properties )
        extension_properties_name_set.insert( extension_properties.extensionName );

    /**
     *
     * Get user config
     *
     * */
    auto required_extension_properties = GlobalConfig::getConfigData( )[ "vulkan" ][ "logical_device_extensions" ];
    return std::ranges::all_of( required_extension_properties.begin( ), required_extension_properties.end( ),
                                [ &extension_properties_name_set ]( const auto& property ) { return extension_properties_name_set.contains( property.template get<std::string>( ) ); } );
}

vk::Format
findSupportedFormat( const vk::PhysicalDevice& physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features )
{
    for ( const auto& format : candidates )
    {
        vk::FormatProperties props = physicalDevice.getFormatProperties( format );

        if ( tiling == vk::ImageTiling::eLinear && ( props.linearTilingFeatures & features ) == features )
        {
            return format;
        } else if ( tiling == vk::ImageTiling::eOptimal && ( props.optimalTilingFeatures & features ) == features )
        {
            return format;
        }
    }

    throw std::runtime_error( "failed to find supported format!" );
}

bool
VulkanAPI::setSwapChainSupportDetails( const vk::PhysicalDevice& device )
{
    m_vkSwap_chain_detail.capabilities = device.getSurfaceCapabilitiesKHR( m_vkSurface.get( ) );
    m_vkSwap_chain_detail.presentModes = device.getSurfacePresentModesKHR( m_vkSurface.get( ) );
    m_vkSwap_chain_detail.formats      = device.getSurfaceFormatsKHR( m_vkSurface.get( ) );

    m_vkSwap_chain_depth_format = findSupportedFormat( device, { vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
                                                       vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment );

    /**
     *
     * Sort by best format
     *
     * */
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

    /**
     *
     * Sort by present mode
     *
     * */
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

        /**
         *
         * Get user config
         *
         * */
        {
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

bool
VulkanAPI::isDeviceUsable( const vk::PhysicalDevice& device )
{
    if ( !device.getFeatures( ).geometryShader ) return false;
    if ( !device.getFeatures( ).multiDrawIndirect ) return false;
    if ( !isDeviceSupportAllExtensions( device ) ) return false;
    if ( !setSwapChainSupportDetails( device ) ) return false;

    return true;
}

void
VulkanAPI::setupLogicalDevice( )
{
    /**
     *
     * Fresh queue family
     *
     * */

    std::vector<vk::QueueFlagBits> queueList { vk::QueueFlagBits::eGraphics, vk::QueueFlagBits::eTransfer };

    // preserved indices
    assert( !( m_requested_queue.contains( reinterpret_cast<void* const>( 0 ) ) && m_requested_queue.contains( reinterpret_cast<void* const>( 1 ) ) ) );

    for ( auto& queue : m_requested_queue )
    {
        queue.second.first = queueList.size( );
        queueList.emplace_back( queue.second.second );
    }

    m_queue_family_manager = std::make_unique<QueueFamilyManager>( m_vkPhysicalDevice );
    m_saved_queue_index    = m_queue_family_manager->GetQueue( queueList );

    // check for surface support
    bool isSurfaceSupport = m_vkPhysicalDevice.getSurfaceSupportKHR( m_saved_queue_index[ 0 ].first, m_vkSurface.get( ) );
    if ( !isSurfaceSupport )
    {
        throw std::runtime_error( "No surface support for the first vk::QueueFlagBits::eGraphics" );
    }

    m_vkQueue_family_indices.graphicsFamily = m_saved_queue_index[ 0 ];
    m_vkQueue_family_indices.presentFamily  = m_saved_queue_index[ 0 ].first;
    m_vkTransfer_family_indices             = m_saved_queue_index[ 1 ];


    std::size_t                                                maxPrioritySize = 0;
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> queue_index_set;
    for ( const auto& pair : m_saved_queue_index )
    {
        queue_index_set[ pair.first ].insert( pair.second );
        maxPrioritySize = std::max( maxPrioritySize, queue_index_set[ pair.first ].size( ) );
    }

    auto queuePriority = std::vector<float>( maxPrioritySize, 1.f );

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::ranges::transform( queue_index_set, std::back_inserter( queueCreateInfos ),
                            [ &queuePriority ]( auto& index ) { return vk::DeviceQueueCreateInfo { vk::DeviceQueueCreateFlags( ), index.first, (uint32_t) index.second.size( ), queuePriority.data( ) }; } );


    /**
     *
     * Create logical device
     *
     * */
    vk::PhysicalDeviceFeatures requiredFeatures { };
    requiredFeatures.multiDrawIndirect = true;

    vk::DeviceCreateInfo createInfo;
    createInfo.setPEnabledFeatures( &requiredFeatures );
    createInfo.setQueueCreateInfoCount( (uint32_t) queueCreateInfos.size( ) );
    createInfo.setQueueCreateInfos( queueCreateInfos );
    createInfo.setEnabledLayerCount( (uint32_t) m_vkValidationLayer->RequiredLayerCount( ) );
    createInfo.setPpEnabledLayerNames( m_vkValidationLayer->RequiredLayerStrPtr( ) );
    createInfo.setEnabledExtensionCount( (uint32_t) m_vkExtension->DeviceExtensionCount( ) );
    createInfo.setPpEnabledExtensionNames( m_vkExtension->DeviceExtensionStrPtr( ) );

    m_vkLogicalDevice = m_vkPhysicalDevice.createDeviceUnique( createInfo );

    /**
     *
     *
     * Setup queues
     *
     * */
    m_vkGraphicQueue = m_vkLogicalDevice->getQueue( m_vkQueue_family_indices.graphicsFamily.first, m_vkQueue_family_indices.graphicsFamily.second );
    m_vkPresentQueue = m_vkLogicalDevice->getQueue( m_vkQueue_family_indices.presentFamily, m_vkQueue_family_indices.graphicsFamily.second );

    Logger::getInstance( ).LogLine( Logger::LogType::eVerbose, "New Graphic queue created", (VkQueue) m_vkGraphicQueue );
    Logger::getInstance( ).LogLine( Logger::LogType::eVerbose, "New Present queue created", (VkQueue) m_vkPresentQueue );
}

void
VulkanAPI::setupSwapChain( )
{

    // index 0 is the best we set
    const auto surface_format = m_vkSwap_chain_detail.formats[ 0 ];
    const auto present_mode   = m_vkSwap_chain_detail.presentModes[ 0 ];
    const auto display_extent = m_vkSwap_chain_detail.getMaxSwapExtent( m_window );
    const auto imageCount     = m_vkSwap_chain_detail.capabilities.minImageCount + 1;
    assert( imageCount <= m_vkSwap_chain_detail.capabilities.maxImageCount );

    uint32_t queueFamilyIndices[] = { m_vkQueue_family_indices.graphicsFamily.first,
                                      m_vkQueue_family_indices.presentFamily };

    /**
     *
     * Create swap chain
     *
     * */

    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.setSurface( m_vkSurface.get( ) );
    createInfo.setMinImageCount( imageCount );
    createInfo.setImageFormat( surface_format.format );
    createInfo.setImageColorSpace( surface_format.colorSpace );
    createInfo.setImageExtent( display_extent );
    createInfo.setImageArrayLayers( 1 );
    createInfo.setImageUsage( vk::ImageUsageFlagBits::eColorAttachment );
    createInfo.setPreTransform( m_vkSwap_chain_detail.capabilities.currentTransform );
    createInfo.setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque );
    createInfo.setPresentMode( present_mode );
    createInfo.setClipped( true );
    createInfo.setOldSwapchain( m_vkSwap_chain.get( ) );
    if ( m_vkQueue_family_indices.graphicsFamily.first != m_vkQueue_family_indices.presentFamily )
    {
        createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = (uint32_t) std::size( queueFamilyIndices );
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    } else
    {
        createInfo.imageSharingMode      = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;         // Optional
        createInfo.pQueueFamilyIndices   = nullptr;   // Optional
    }

    m_vkSwap_chain = m_vkLogicalDevice->createSwapchainKHRUnique( createInfo );

    /**
     *
     * Create swap chain image and image view
     *
     * */
    m_vkSwap_chain_images = m_vkLogicalDevice->getSwapchainImagesKHR( m_vkSwap_chain.get( ) );
    m_vkSwap_chain_image_views.clear( );
    m_vkSwap_chain_image_views.reserve( m_vkSwap_chain_images.size( ) );
    std::ranges::transform( m_vkSwap_chain_images, std::back_inserter( m_vkSwap_chain_image_views ),
                            [ this, &surface_format ]( const vk::Image& image ) {
                                vk::ImageViewCreateInfo createInfo;
                                createInfo.setImage( image );
                                createInfo.setViewType( vk::ImageViewType::e2D );
                                createInfo.setFormat( surface_format.format );
                                createInfo.setComponents( { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity } );
                                createInfo.setSubresourceRange( { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } );

                                return std::move( m_vkLogicalDevice->createImageViewUnique( createInfo ) );
                            } );

    m_vkSwap_chain_depth_image.SetAllocator( m_vkmAllocator );
    m_vkSwap_chain_depth_image.Create( display_extent.width, display_extent.height, m_vkSwap_chain_depth_format, vk::ImageTiling::eOptimal,
                                       vk::ImageUsageFlagBits::eDepthStencilAttachment, VMA_MEMORY_USAGE_GPU_ONLY );

    m_vkSwap_chain_depth_image.CreateImageView( m_vkSwap_chain_depth_format, vk::ImageAspectFlagBits::eDepth );
}

std::string
FindResourcePath( )
{
    static constexpr auto searchDepth       = 10;
    static bool           foundResourcePath = false;
    static std::string    resourcePath;

    if ( resourcePath.empty( ) )
    {
        resourcePath = "Resources";
        for ( int i = 0; i < searchDepth; ++i )
        {
            Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Checking resource path " + resourcePath );
            if ( !std::filesystem::exists( std::filesystem::path( resourcePath ) ) )
            {
                resourcePath.insert( 0, "../" );
                continue;
            }

            foundResourcePath = true;
            return resourcePath;
        }
    } else if ( foundResourcePath )
    {
        Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Using cached resource path " + resourcePath );
        return resourcePath;
    }

    throw std::runtime_error( "Can't locate resource path" );
}

void
VulkanAPI::setupPipeline( )
{

    m_vkDisplayExtent = m_vkSwap_chain_detail.getMaxSwapExtent( m_window );

    /**
     *
     * Create and load shader
     *
     * */
    std::unique_ptr<VulkanShader> shader = std::make_unique<VulkanShader>( );

    std::string resourcePath = FindResourcePath( );
    shader->InitGLSLFile( m_vkLogicalDevice.get( ), resourcePath + "/Shader/vertex_buffer.vert", resourcePath + "/Shader/vertex_buffer.frag" );

    /**
     *
     * Create pipeline
     *
     * */
    m_vkPipeline = std::make_unique<VulkanPipeline>( std::move( shader ) );
    m_vkPipeline->Create<DataType::TexturedVertex>( (float) m_vkDisplayExtent.width,
                                                    (float) m_vkDisplayExtent.height,
                                                    static_cast<uint32_t>( getSwapChainImagesCount( ) ),
                                                    m_vkLogicalDevice.get( ),
                                                    m_vkSwap_chain_detail.formats[ 0 ],
                                                    m_vkSwap_chain_depth_format );

    /**
     *
     * Create frame buffers
     *
     * */
    m_vkFrameBuffers.clear( );
    m_vkFrameBuffers.reserve( m_vkSwap_chain_image_views.size( ) );
    std::ranges::transform( m_vkSwap_chain_image_views, std::back_inserter( m_vkFrameBuffers ), [ this ]( const auto& image_view ) {
        std::vector<vk::ImageView> attachments { image_view.get( ), m_vkSwap_chain_depth_image.GetImageView( ) };
        vk::FramebufferCreateInfo  framebufferInfo { };
        framebufferInfo.setRenderPass( m_vkPipeline->getRenderPass( ) );
        framebufferInfo.setAttachments( attachments );
        framebufferInfo.setWidth( m_vkDisplayExtent.width );
        framebufferInfo.setHeight( m_vkDisplayExtent.height );
        framebufferInfo.setLayers( 1 );

        return m_vkLogicalDevice->createFramebufferUnique( framebufferInfo );
    } );

    if ( m_pipeline_create_callback ) m_pipeline_create_callback( );
}

void
VulkanAPI::setupGraphicCommand( )
{
    /**
     *
     * Creation of command pool
     *
     * */

    Logger::getInstance( ).LogLine( Logger::LogType::eWarn, "Using vk::CommandPoolCreateFlagBits::eResetCommandBuffer" );
    m_vkGraphicCommandPool  = m_vkLogicalDevice->createCommandPoolUnique( { { vk::CommandPoolCreateFlagBits::eResetCommandBuffer }, m_vkQueue_family_indices.graphicsFamily.first } );
    m_vkTransferCommandPool = m_vkLogicalDevice->createCommandPoolUnique( { { vk::CommandPoolCreateFlagBits::eResetCommandBuffer }, m_vkTransfer_family_indices.first } );

    setupGraphicCommandBuffers( );
}

void
VulkanAPI::setupSyncs( )
{
    m_sync_count = GlobalConfig::getConfigData( )[ "sync_frame_count" ].get<uint32_t>( );

    m_vkImage_acquire_syncs.clear( );
    m_vkImage_acquire_syncs.reserve( m_sync_count );
    m_vkRender_syncs.clear( );
    m_vkRender_syncs.reserve( m_sync_count );
    m_vkRender_fence_syncs.clear( );
    m_vkRender_fence_syncs.reserve( m_sync_count );
    m_vkSwap_chain_image_fence_syncs.clear( );
    m_vkSwap_chain_image_fence_syncs.resize( m_vkFrameBuffers.size( ), nullptr );

    for ( uint32_t i = 0; i < m_sync_count; ++i )
    {
        m_vkImage_acquire_syncs.emplace_back( m_vkLogicalDevice->createSemaphoreUnique( { } ) );
        m_vkRender_syncs.emplace_back( m_vkLogicalDevice->createSemaphoreUnique( { } ) );
        m_vkRender_fence_syncs.emplace_back( m_vkLogicalDevice->createFenceUnique( { vk::FenceCreateFlagBits::eSignaled } ) );
    }
}

uint32_t
VulkanAPI::acquireNextImage( )
{
    if ( m_swap_chain_not_valid.test( ) )
    {
        adeptSwapChainChange( );
        m_swap_chain_not_valid.clear( );
    }

    static auto next_image = [ this ]( ) { return m_vkLogicalDevice->acquireNextImageKHR( m_vkSwap_chain.get( ), std::numeric_limits<uint64_t>::max( ), m_vkImage_acquire_syncs[ m_sync_index ].get( ), nullptr ); };
    auto        result     = next_image( );
    while ( result.result == vk::Result::eErrorOutOfDateKHR )
    {
        Logger::getInstance( ).LogLine( Logger::LogType::eError, "Recreating swap chain!!!" );
        adeptSwapChainChange( );
        result = next_image( );
    }

    return result.value;
}

void
VulkanAPI::cycleGraphicCommandBuffers( uint32_t index )
{
    const vk::Extent2D display_extent = m_vkSwap_chain_detail.getMaxSwapExtent( m_window );
    size_t             it_index, end_index;
    if ( index == (uint32_t) -1 )
    {

        it_index  = 0;
        end_index = m_vkGraphicCommandBuffers.size( );
    } else
    {
        assert( index < m_vkGraphicCommandBuffers.size( ) );
        it_index  = index;
        end_index = index + 1;
    }

    for ( ; it_index != end_index; ++it_index )
    {
        // implicit call
        // m_vkGraphicCommandBuffers[ it_index ].reset( );
        m_vkGraphicCommandBuffers[ it_index ].begin( { vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );

        vk::RenderPassBeginInfo render_pass_begin_info;
        render_pass_begin_info.setRenderPass( m_vkPipeline->getRenderPass( ) );
        render_pass_begin_info.setFramebuffer( m_vkFrameBuffers[ it_index ].get( ) );
        render_pass_begin_info.setRenderArea( {
            {0, 0},
            display_extent
        } );
        render_pass_begin_info.setClearValues( m_clearValues );

        m_vkGraphicCommandBuffers[ it_index ].beginRenderPass( render_pass_begin_info, vk::SubpassContents::eInline );
        m_vkGraphicCommandBuffers[ it_index ].bindPipeline( vk::PipelineBindPoint::eGraphics, m_vkPipeline->getPipeline( ) );

        /**
         *
         * Rendering
         *
         * */
        m_renderer( m_vkGraphicCommandBuffers[ it_index ], (uint32_t) it_index );

        m_vkGraphicCommandBuffers[ it_index ].endRenderPass( );
        m_vkGraphicCommandBuffers[ it_index ].end( );
    }
}

void
VulkanAPI::adeptSwapChainChange( )
{
    m_should_create_swap_chain.wait( false );
    m_vkLogicalDevice->waitIdle( );

    setSwapChainSupportDetails( m_vkPhysicalDevice );

    /**
     *
     * Setup render detail
     *
     * */
    setupSwapChain( );
    setupPipeline( );

    /**
     *
     * Graphic command setup ( only command buffer )
     *
     * */
    setupGraphicCommandBuffers( );
    // cycleGraphicCommandBuffers( );
}

void
VulkanAPI::setupGraphicCommandBuffers( )
{
    m_vkGraphicCommandBuffers.clear( );

    /**
     *
     * Creation of command buffers
     *
     * */
    vk::CommandBufferAllocateInfo allocInfo { };
    allocInfo.setCommandPool( m_vkGraphicCommandPool.get( ) );
    allocInfo.setLevel( vk::CommandBufferLevel::ePrimary );
    allocInfo.setCommandBufferCount( (uint32_t) m_vkFrameBuffers.size( ) );

    m_vkGraphicCommandBuffers = m_vkLogicalDevice->allocateCommandBuffers( allocInfo );
}

VulkanAPI::~VulkanAPI( )
{
    m_vkSwap_chain_depth_image.DestroyBuffer( );
    vmaDestroyAllocator( m_vkmAllocator );
}

void
VulkanAPI::setupVulkanMemoryAllocator( )
{
    VmaAllocatorCreateInfo allocatorInfo = { };
    allocatorInfo.physicalDevice         = m_vkPhysicalDevice;
    allocatorInfo.device                 = m_vkLogicalDevice.get( );
    allocatorInfo.instance               = m_vkInstance.get( );

    vmaCreateAllocator( &allocatorInfo, &m_vkmAllocator );
}
