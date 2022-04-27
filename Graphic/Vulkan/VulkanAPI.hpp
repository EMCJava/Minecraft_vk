//
// Created by loys on 1/24/2022.
//

#ifndef MINECRAFT_VK_VULKAN_VULKANAPI_HPP
#define MINECRAFT_VK_VULKAN_VULKANAPI_HPP

#include <Include/GraphicAPI.hpp>

#include <Graphic/Vulkan/Pipeline/VulkanPipeline.hpp>
#include <Utility/Vulkan/ValidationLayer.hpp>
#include <Utility/Vulkan/VulkanExtension.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace DataType
{

struct ColoredVertex : VertexDetail {
    glm::vec2 pos;
    glm::vec3 color;

    ColoredVertex(glm::vec2 p, glm::vec3 c) : pos(p), color(c) {}

    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions( )
    {
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions( 2 );

        attributeDescriptions[ 0 ].setBinding( 0 );
        attributeDescriptions[ 0 ].setLocation( 0 );
        attributeDescriptions[ 0 ].setFormat( vk::Format::eR32G32Sfloat );
        attributeDescriptions[ 0 ].setOffset( offsetof( ColoredVertex, pos ) );

        attributeDescriptions[ 1 ].setBinding( 0 );
        attributeDescriptions[ 1 ].setLocation( 1 );
        attributeDescriptions[ 1 ].setFormat( vk::Format::eR32G32B32Sfloat );
        attributeDescriptions[ 1 ].setOffset( offsetof( ColoredVertex, color ) );

        return attributeDescriptions;
    }

    static std::vector<vk::VertexInputBindingDescription> getBindingDescriptions( )
    {
        std::vector<vk::VertexInputBindingDescription> bindingDescription( 1 );

        bindingDescription[ 0 ].setBinding( 0 );
        bindingDescription[ 0 ].setStride( sizeof( ColoredVertex ) );
        bindingDescription[ 0 ].setInputRate( vk::VertexInputRate::eVertex );

        return bindingDescription;
    }
};

}   // namespace DataType

class VulkanAPI
{
private:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        [[nodiscard]] bool isComplete( ) const
        {
            return graphicsFamily.has_value( ) && presentFamily.has_value( );
        }
    };

    struct SwapChainSupportDetails {

        vk::SurfaceCapabilitiesKHR        capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR>   presentModes;

        [[nodiscard]] vk::Extent2D getMaxSwapExtent( GLFWwindow* window ) const;
        [[nodiscard]] bool         isComplete( ) const
        {
            return !formats.empty( ) && !presentModes.empty( );
        }
    };

    /**
     *
     * This function will call setSwapChainSupportDetails
     *
     * */
    bool isDeviceUsable( const vk::PhysicalDevice& device );
    bool isDeviceSupportAllExtensions( const vk::PhysicalDevice& device );
    bool setSwapChainSupportDetails( const vk::PhysicalDevice& device );

    void updateQueueFamiliesIndex( const vk::PhysicalDevice& );

    void setupDynamicDispatch( );

    void setupValidationLayer( );
    void setupExtensions( );
    void setupSurface( );

    void selectPhysicalDevice( );
    void setupLogicalDevice( );

    void adeptSwapChainChange( );
    void setupSwapChain( );
    void setupPipeline( );

    /**
     *
     * Also setup command buffer
     *
     * */
    void setupGraphicCommand( );
    void setupGraphicCommandBuffers( );

    void setupSyncs( );

public:
    explicit VulkanAPI( GLFWwindow* windows );

    void setupAPI( std::string applicationName );

    [[nodiscard]] uint32_t acquireNextImage( );
    void                   setRenderer( std::function<void( const vk::CommandBuffer& )>&& renderer ) { m_renderer = std::move( renderer ); };

    /*
     *
     * Run(cycle) renderer on command buffer
     *
     * */
    void cycleGraphicCommandBuffers( bool cycle_all_buffer = true, uint32_t index = 0 );
    void presentFrame( uint32_t index = -1 );

    /*
     *
     * sync function
     *
     * */
    inline void waitPresent( ) { m_vkPresentQueue.waitIdle( ); }
    inline void waitIdle( ) const { m_vkLogicalDevice->waitIdle( ); }

    inline vk::Device& getLogicalDevice() { return m_vkLogicalDevice.get(); }
    inline vk::PhysicalDevice& getPhysicalDevice() { return m_vkPhysicalDevice; }

    inline void invalidateSwapChain( ) { m_swap_chain_not_valid.test_and_set( ); }
    inline void setShouldCreateSwapChain( bool should )
    {
        if ( should )
        {
            m_should_create_swap_chain.test_and_set( );
            m_should_create_swap_chain.notify_one( );
        } else
        {
            m_should_create_swap_chain.clear( );
        }
    }

private:
    /**
     *
     * Window handler
     *
     * */
    GLFWwindow* m_window;

    /**
     *
     * Custom properties
     *
     * */
    std::unique_ptr<ValidationLayer> m_vkValidationLayer;
    std::unique_ptr<VulkanExtension> m_vkExtension;


    /**
     *
     * Instances
     *
     * */
    vk::UniqueInstance        m_vkInstance;
    vk::DispatchLoaderDynamic m_vkDynamicDispatch;

    /**
     *
     * Debug manager
     *
     * */
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> m_vkDebugMessenger;
    void                                                                    setupDebugManager( const vk::DebugUtilsMessengerCreateInfoEXT& );

    /**
     *
     * Physical properties
     *
     * */
    vk::UniqueSurfaceKHR m_vkSurface;
    vk::UniqueDevice     m_vkLogicalDevice;
    vk::PhysicalDevice   m_vkPhysicalDevice;

    /**
     *
     * Queues
     *
     * */
    QueueFamilyIndices m_vkQueue_family_indices;
    vk::Queue          m_vkGraphicQueue;
    vk::Queue          m_vkPresentQueue;

    /**
     *
     * Swap chains
     *
     * */
    std::atomic_flag                 m_swap_chain_not_valid;
    std::atomic_flag                 m_should_create_swap_chain;
    SwapChainSupportDetails          m_vkSwap_chain_detail;
    vk::UniqueSwapchainKHR           m_vkSwap_chain;
    std::vector<vk::Image>           m_vkSwap_chain_images;
    std::vector<vk::UniqueImageView> m_vkSwap_chain_image_views;

    /**
     *
     * Pipelines
     *
     * */
    std::unique_ptr<VulkanPipeline>    m_vkPipeline;
    std::vector<vk::UniqueFramebuffer> m_vkFrameBuffers;

    /**
     *
     * Commands
     *
     * */
    vk::UniqueCommandPool          m_vkGraphicCommandPool;
    std::vector<vk::CommandBuffer> m_vkGraphicCommandBuffers;

    /**
     *
     * Synchronization
     *
     * */
    uint32_t                         m_sync_index = 0, m_sync_count = 1;
    std::vector<vk::UniqueSemaphore> m_vkImage_acquire_syncs;
    std::vector<vk::UniqueSemaphore> m_vkRender_syncs;
    std::vector<vk::UniqueFence>     m_vkRender_fence_syncs;
    std::vector<vk::Fence>           m_vkSwap_chain_image_fence_syncs;

    /**
     *
     * Custom detail(s)
     *
     * */
    std::function<void( const vk::CommandBuffer& )> m_renderer;
};


#endif   // MINECRAFT_VK_VULKAN_VULKANAPI_HPP
