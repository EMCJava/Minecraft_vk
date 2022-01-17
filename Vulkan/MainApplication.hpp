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

        bool isComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

private:
    GLFWwindow* m_window;
    int m_screen_width, m_screen_height;
    bool m_window_resizable = false;

    std::unique_ptr<ValidationLayer> m_vkValidationLayer;
    std::unique_ptr<VulkanExtension> m_vkExtension;

    vk::Queue m_vkGraphicQueue;
    vk::Queue m_vkPresentQueue;

    vk::DispatchLoaderDynamic m_vkDynamicDispatch;
    vk::DebugUtilsMessengerEXT m_vkdebugMessenger;
    vk::UniqueInstance m_vkInstance;

    vk::UniqueSurfaceKHR m_vksurface;
    vk::PhysicalDevice m_vkPhysicalDevice;
    vk::UniqueDevice m_vkLogicalDevice;

    void graphicAPIInfo();

    void InitWindow();
    void InitGraphicAPI();

    QueueFamilyIndices findQueueFamilies();

    void selectPhysicalDevice();
    void createLogicalDevice();

    void cleanUp();

public:
    MainApplication();
    ~MainApplication();

    void run();
};

#endif   // MINECRAFT_VK_VULKAN_MAINAPPLICATION_HPP
