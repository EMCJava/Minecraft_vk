//
// Created by loys on 16/1/2022.
//

#ifndef MINECRAFT_VK_VULKAN_MAINAPPLICATION_HPP
#define MINECRAFT_VK_VULKAN_MAINAPPLICATION_HPP

#include <Include/GraphicAPI.hpp>

#include <Graphic/Vulkan/Pipeline/VulkanPipeline.hpp>
#include <Graphic/Vulkan/VulkanAPI.hpp>
#include <Include/GlobalConfig.hpp>
#include <Utility/Logger.hpp>
#include <Utility/Vulkan/ValidationLayer.hpp>
#include <Utility/Vulkan/VulkanExtension.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vulkan/vulkan_handles.hpp>

class MainApplication
{

    GLFWwindow*                m_window { };
    int                        m_screen_width { }, m_screen_height { };
    bool                       m_window_resizable = false;

    std::unique_ptr<VulkanAPI> m_graphics_api;

    void                       InitWindow( );
    void                       cleanUp( );

    std::atomic<bool>          m_render_thread_should_run;
    void renderThread();

public:
    MainApplication( );
    ~MainApplication( );

    void run( );
};

#endif   // MINECRAFT_VK_VULKAN_MAINAPPLICATION_HPP
