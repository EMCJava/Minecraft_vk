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
    GLFWwindow* m_window;
    int m_screen_width, m_screen_height;
    bool m_window_resizable = false;

    ValidationLayer m_vkValidationLayer;
    VulkanExtension m_vkExtension;

    vk::DispatchLoaderDynamic m_vkDynamicDispatch;
    vk::DebugUtilsMessengerEXT m_vkdebugMessenger;
    vk::Instance m_vkInstance;

    void graphicAPIInfo();

    void InitWindow();
    void InitGraphicAPI();

    void cleanUp();

public:
    MainApplication();
    ~MainApplication();

    void run();
};

#endif // MINECRAFT_VK_VULKAN_MAINAPPLICATION_HPP
