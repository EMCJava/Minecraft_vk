#include "VulkanAPI.hpp"
//
// Created by samsa on 1/24/2022.
//

#include <Utility/Logger.hpp>

#include <unordered_set>

#include "VulkanAPI.hpp"

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
VulkanAPI::setupAPI( std::string applicationName )
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
    createInfo.setEnabledLayerCount( m_vkValidationLayer->RequiredLayerCount( ) );
    createInfo.setPpEnabledLayerNames( m_vkValidationLayer->RequiredLayerStrPtr( ) );
    createInfo.setEnabledExtensionCount( m_vkExtension->VulkanExtensionCount( ) );
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
        Logger::getInstance( ).LogLine( "Found device: [", vk::to_string( device_properties.deviceType ), "]", device_properties.deviceName );

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
        const auto device_properties = m_vkPhysicalDevice.getProperties( );

        Logger::getInstance( ).LogLine( "Using device: [", vk::to_string( device_properties.deviceType ), "]", device_properties.deviceName );
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
    auto required_extension_properties = GlobalConfig::getConfigData( )[ "logical_device_extensions" ];
    return std::ranges::all_of( required_extension_properties.begin( ), required_extension_properties.end( ),
                                [ &extension_properties_name_set ]( const auto& property ) { return extension_properties_name_set.contains( property.template get<std::string>( ) ); } );
}

bool
VulkanAPI::setSwapChainSupportDetails( const vk::PhysicalDevice& device )
{
    m_vkSwap_chain_detail.capabilities = device.getSurfaceCapabilitiesKHR( m_vkSurface.get( ) );
    m_vkSwap_chain_detail.presentModes = device.getSurfacePresentModesKHR( m_vkSurface.get( ) );
    m_vkSwap_chain_detail.formats      = device.getSurfaceFormatsKHR( m_vkSurface.get( ) );

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
    updateQueueFamiliesIndex( m_vkPhysicalDevice );

    std::unordered_set<uint32_t> queue_index_set { m_vkQueue_family_indices.graphicsFamily.value( ), m_vkQueue_family_indices.presentFamily.value( ) };
    float                        queuePriority = 1.0f;

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::ranges::transform( queue_index_set, std::back_inserter( queueCreateInfos ),
                            [ queuePriority = &queuePriority ]( uint32_t index ) { return vk::DeviceQueueCreateInfo { vk::DeviceQueueCreateFlags( ), index, 1, queuePriority }; } );


    /**
     *
     * Create logical device
     *
     * */
    // vk::PhysicalDeviceFeatures requiredFeatures { };
    vk::DeviceCreateInfo createInfo;
    createInfo.setQueueCreateInfoCount( queueCreateInfos.size( ) );
    createInfo.setQueueCreateInfos( queueCreateInfos );
    createInfo.setEnabledLayerCount( m_vkValidationLayer->RequiredLayerCount( ) );
    createInfo.setPpEnabledLayerNames( m_vkValidationLayer->RequiredLayerStrPtr( ) );
    createInfo.setEnabledExtensionCount( m_vkExtension->DeviceExtensionCount( ) );
    createInfo.setPpEnabledExtensionNames( m_vkExtension->DeviceExtensionStrPtr( ) );

    m_vkLogicalDevice = m_vkPhysicalDevice.createDeviceUnique( createInfo );

    /**
     *
     *
     * Setup queues
     *
     * */
    m_vkGraphicQueue = m_vkLogicalDevice->getQueue( m_vkQueue_family_indices.graphicsFamily.value( ), 0 );
    m_vkPresentQueue = m_vkLogicalDevice->getQueue( m_vkQueue_family_indices.presentFamily.value( ), 0 );
}

void
VulkanAPI::updateQueueFamiliesIndex( const vk::PhysicalDevice& phy_device )
{

    m_vkQueue_family_indices.graphicsFamily.reset( );
    m_vkQueue_family_indices.presentFamily.reset( );

    auto queueFamilies = phy_device.getQueueFamilyProperties( );

    for ( int i = 0; i < queueFamilies.size( ); ++i )
    {
        if ( queueFamilies[ i ].queueCount > 0 && queueFamilies[ i ].queueFlags & vk::QueueFlagBits::eGraphics )
        {
            m_vkQueue_family_indices.graphicsFamily = i;
        }

        if ( queueFamilies[ i ].queueCount > 0 && phy_device.getSurfaceSupportKHR( i, m_vkSurface.get( ) ) )
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
VulkanAPI::setupSwapChain( )
{

    // index 0 is the best we set
    const auto surface_format = m_vkSwap_chain_detail.formats[ 0 ];
    const auto present_mode   = m_vkSwap_chain_detail.presentModes[ 0 ];
    const auto display_extent = m_vkSwap_chain_detail.getMaxSwapExtent( m_window );
    const auto imageCount     = m_vkSwap_chain_detail.capabilities.minImageCount + 1;
    assert( imageCount <= m_vkSwap_chain_detail.capabilities.maxImageCount );

    uint32_t queueFamilyIndices[] = { m_vkQueue_family_indices.graphicsFamily.value( ),
                                      m_vkQueue_family_indices.presentFamily.value( ) };

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
}

void
VulkanAPI::setupPipeline( )
{

    const VkExtent2D display_extent = m_vkSwap_chain_detail.getMaxSwapExtent( m_window );

    /**
     *
     * Create and load shader
     *
     * */
    std::unique_ptr<VulkanShader> shader = std::make_unique<VulkanShader>( );
    shader->InitGLSLFile( m_vkLogicalDevice.get( ), "../Resources/Shader/vertex_buffer.vert", "../Resources/Shader/vertex_buffer.frag" );

    /**
     *
     * Create pipeline
     *
     * */
    m_vkPipeline = std::make_unique<VulkanPipeline>( std::move( shader ) );
    m_vkPipeline->Create<DataType::ColoredVertex>( display_extent.width, display_extent.height, m_vkLogicalDevice.get( ), m_vkSwap_chain_detail.formats[ 0 ] );

    /**
     *
     * Create frame buffers
     *
     * */
    m_vkFrameBuffers.clear( );
    m_vkFrameBuffers.reserve( m_vkSwap_chain_image_views.size( ) );
    std::ranges::transform( m_vkSwap_chain_image_views, std::back_inserter( m_vkFrameBuffers ), [ display_extent, this ]( const auto& image_view ) {
        vk::FramebufferCreateInfo framebufferInfo { };
        framebufferInfo.setRenderPass( m_vkPipeline->getRenderPass( ) );
        framebufferInfo.setAttachmentCount( 1 );
        framebufferInfo.setAttachments( image_view.get( ) );
        framebufferInfo.setWidth( display_extent.width );
        framebufferInfo.setHeight( display_extent.height );
        framebufferInfo.setLayers( 1 );

        return m_vkLogicalDevice->createFramebufferUnique( framebufferInfo );
    } );
}

void
VulkanAPI::setupGraphicCommand( )
{
    /**
     *
     * Creation of command pool
     *
     * */
    m_vkGraphicCommandPool = m_vkLogicalDevice->createCommandPoolUnique( { { }, m_vkQueue_family_indices.graphicsFamily.value( ) } );

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

    for ( int i = 0; i < m_sync_count; ++i )
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
        Logger::getInstance( ).LogLine( Logger::Color::eRed, "Recreating swap chain!!!" );
        adeptSwapChainChange( );
        result = next_image( );
    }

    return result.value;
}

void
VulkanAPI::cycleGraphicCommandBuffers( bool cycle_all_buffer, uint32_t index )
{
    const VkExtent2D display_extent = m_vkSwap_chain_detail.getMaxSwapExtent( m_window );
    size_t           it_index, end_index;
    if ( cycle_all_buffer )
    {

        it_index  = 0;
        end_index = m_vkGraphicCommandBuffers.size( );
    } else
    {
        assert( index < m_vkGraphicCommandBuffers.size( ) );
        it_index  = index;
        end_index = index + 1;
    }

    vk::ClearValue clearColor { { std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } } };
    for ( ; it_index != end_index; ++it_index )
    {
        m_vkGraphicCommandBuffers[ it_index ].begin( { vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr } );

        vk::RenderPassBeginInfo render_pass_begin_info;
        render_pass_begin_info.setRenderPass( m_vkPipeline->getRenderPass( ) );
        render_pass_begin_info.setFramebuffer( m_vkFrameBuffers[ it_index ].get( ) );
        render_pass_begin_info.setRenderArea( {
            {0, 0},
            display_extent
        } );
        render_pass_begin_info.setClearValueCount( 1 );
        render_pass_begin_info.setClearValues( clearColor );

        m_vkGraphicCommandBuffers[ it_index ].beginRenderPass( render_pass_begin_info, vk::SubpassContents::eInline );
        m_vkGraphicCommandBuffers[ it_index ].bindPipeline( vk::PipelineBindPoint::eGraphics, m_vkPipeline->getPipeline( ) );

        /**
         *
         * Rendering
         *
         * */
        m_renderer( m_vkGraphicCommandBuffers[ it_index ] );

        m_vkGraphicCommandBuffers[ it_index ].endRenderPass( );
        m_vkGraphicCommandBuffers[ it_index ].end( );
    }
}

void
VulkanAPI::presentFrame( uint32_t index )
{
    /**
     *
     * Sync in renderer
     *
     * */
    auto wait_fence_result = m_vkLogicalDevice->waitForFences( m_vkRender_fence_syncs[ m_sync_index ].get( ), true, std::numeric_limits<uint64_t>::max( ) );
    assert( wait_fence_result == vk::Result::eSuccess );

    /**
     *
     * Sync in swap chain image
     * In-case too many synced render on the same image
     *
     * */
    if ( m_vkSwap_chain_image_fence_syncs[ index ] )
    {
        wait_fence_result = m_vkLogicalDevice->waitForFences( m_vkSwap_chain_image_fence_syncs[ m_sync_index ], true, std::numeric_limits<uint64_t>::max( ) );
        assert( wait_fence_result == vk::Result::eSuccess );
    }

    m_vkSwap_chain_image_fence_syncs[ index ] = m_vkRender_fence_syncs[ m_sync_index ].get( );

    /**
     *
     * Reset fence to unsignaled state
     *
     * */
    m_vkLogicalDevice->resetFences( m_vkRender_fence_syncs[ m_sync_index ].get( ) );

    /**
     *
     * Submit
     *
     * */
    vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo         submitInfo {
        1, &m_vkImage_acquire_syncs[ m_sync_index ].get( ), &waitStageMask,
        1, &m_vkGraphicCommandBuffers[ index ],
        1, &m_vkRender_syncs[ m_sync_index ].get( ) };

    m_vkGraphicQueue.submit( submitInfo, m_vkRender_fence_syncs[ m_sync_index ].get( ) );

    /**
     *
     * Present
     *
     * */
    vk::PresentInfoKHR presentInfo { 1, &m_vkRender_syncs[ m_sync_index ].get( ),
                                     1, &m_vkSwap_chain.get( ), &index };


    try
    {
        const auto present_result = m_vkPresentQueue.presentKHR( presentInfo );
        if ( present_result == vk::Result::eSuboptimalKHR ) adeptSwapChainChange( );
        assert( present_result == vk::Result::eSuccess );
    }
    catch ( vk::OutOfDateKHRError& err )
    {
        m_swap_chain_not_valid.test_and_set( );
    }

    m_sync_index = ( m_sync_index + 1 ) % m_sync_count;
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
    cycleGraphicCommandBuffers( );
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
    allocInfo.setCommandBufferCount( m_vkFrameBuffers.size( ) );

    m_vkGraphicCommandBuffers = m_vkLogicalDevice->allocateCommandBuffers( allocInfo );
}
