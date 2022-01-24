//
// Created by samsa on 1/24/2022.
//

#ifndef MINECRAFT_VK_VULKAN_VULKANAPI_HPP
#define MINECRAFT_VK_VULKAN_VULKANAPI_HPP

#include "Include/GraphicAPI.hpp"

#include <Graphic/Vulkan/Pipeline/VulkanPipeline.hpp>
#include <Utility/Vulkan/ValidationLayer.hpp>
#include <Utility/Vulkan/VulkanExtension.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

class VulkanAPI
{
private:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        [[nodiscard]] bool      isComplete( ) const
        {
            return graphicsFamily.has_value( ) && presentFamily.has_value( );
        }
    };

    struct SwapChainSupportDetails {

        vk::SurfaceCapabilitiesKHR        capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR>   presentModes;

        [[nodiscard]] vk::Extent2D        getMaxSwapExtent( GLFWwindow* window ) const;
        [[nodiscard]] bool                isComplete( ) const
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

    void setupSwapChain( );
    void setupPipeline( );

    void setupGraphicCommand( );

    void setupSyncs( );

public:
    explicit VulkanAPI( GLFWwindow* windows );

    void                   setupAPI( std::string applicationName );

    [[nodiscard]] uint32_t acquireNextImage( ) const;
    void                   cycleGraphicCommandBuffers( const std::function<void( const vk::CommandBuffer& )>& renderer, bool cycle_all_buffer = true, uint32_t index = 0 );
    void                   presentFrame( uint32_t index = -1 );

    inline void            waitPresent( ) { m_vkPresentQueue.waitIdle( ); }
    inline void            waitIdle( ) const { m_vkLogicalDevice->waitIdle( ); }

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
    vk::UniqueSemaphore m_vkImage_acquire_sync;
    vk::UniqueSemaphore m_vkRender_sync;
};


#endif   // MINECRAFT_VK_VULKAN_VULKANAPI_HPP
