//
// Created by loys on 16/1/2022.
//

#ifndef MINECRAFT_VK_VULKAN_MAINAPPLICATION_HPP
#define MINECRAFT_VK_VULKAN_MAINAPPLICATION_HPP

#include "GraphicAPI.hpp"

#include <Include/GlobalConfig.hpp>
#include <Utility/Logger.hpp>
#include <Utility/Vulkan/ValidationLayer.hpp>
#include <Utility/Vulkan/VulkanExtension.hpp>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

class MainApplication
{
private:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool                    isComplete( )
        {
            return graphicsFamily.has_value( ) && presentFamily.has_value( );
        }
    };

    struct SwapChainSupportDetails {

        vk::SurfaceCapabilitiesKHR        capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR>   presentModes;

        vk::Extent2D                      getMaxSwapExtent( GLFWwindow* window ) const;

        [[nodiscard]] bool                isComplete( ) const
        {
            return !formats.empty( ) && !presentModes.empty( );
        }
    };


private:
    GLFWwindow*                      m_window { };
    int                              m_screen_width { }, m_screen_height { };
    bool                             m_window_resizable = false;

    std::unique_ptr<ValidationLayer> m_vkValidationLayer;
    std::unique_ptr<VulkanExtension> m_vkExtension;

    vk::UniqueInstance               m_vkInstance;

    vk::UniqueSurfaceKHR             m_vkSurface;
    vk::UniqueDevice                 m_vkLogicalDevice;
    vk::PhysicalDevice               m_vkPhysicalDevice;

    vk::Queue                        m_vkGraphicQueue;
    vk::Queue                        m_vkPresentQueue;

    QueueFamilyIndices               m_vkQueue_family_indices;

    SwapChainSupportDetails          m_vkSwap_chain_detail;
    vk::UniqueSwapchainKHR           m_vkSwap_chain;
    std::vector<vk::Image>           m_vkSwap_chain_images;
    // std::vector<vk::UniqueImageView> m_vkSwap_chain_image_views;
    std::vector<vk::UniqueImageView> m_vkSwap_chain_image_views;

    // Debug
    vk::DispatchLoaderDynamic  m_vkDynamicDispatch;
    vk::DebugUtilsMessengerEXT m_vkDebugMessenger;

    static void                graphicAPIInfo( );
    static bool                checkDeviceExtensionSupport( const vk::PhysicalDevice& device );

    bool                       setSwapChainSupportDetails( const vk::PhysicalDevice& device );
    void                       createSwapChain( );

    void                       InitWindow( );
    void                       InitGraphicAPI( );

    void                       setQueueFamiliesIndex( );

    void                       selectPhysicalDevice( );
    void                       createLogicalDevice( );

    void                       cleanUp( );

public:
    MainApplication( );
    ~MainApplication( );

    void run( );
};

#endif   // MINECRAFT_VK_VULKAN_MAINAPPLICATION_HPP
