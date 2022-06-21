//
// Created by loys on 16/1/2022.
//

#ifndef MINECRAFT_VK_VULKAN_MAINAPPLICATION_HPP
#define MINECRAFT_VK_VULKAN_MAINAPPLICATION_HPP

#include <Include/GraphicAPI.hpp>

#include <Graphic/Vulkan/Pipeline/VulkanPipeline.hpp>
#include <Graphic/Vulkan/VulkanAPI.hpp>
#include <Include/GlobalConfig.hpp>
#include <Minecraft/Minecraft.hpp>
#include <Utility/Logger.hpp>
#include <Utility/Singleton.hpp>
#include <Utility/Vulkan/ValidationLayer.hpp>
#include <Utility/Vulkan/VulkanExtension.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vulkan/vulkan_handles.hpp>

class MainApplication : public Singleton<MainApplication>
{

    GLFWwindow* m_window { };
    int         m_backup_screen_width { }, m_backup_screen_height { };
    int         m_screen_width { }, m_screen_height { };
    int         m_screen_pos_x { }, m_screen_pos_y { };
    bool        m_window_resizable  = false;
    bool        m_window_fullscreen = false;
    bool        m_is_mouse_locked   = false;

    std::pair<double, double> m_MousePos;

    std::atomic_flag          m_deltaMouseHoldUpdate;
    std::pair<double, double> m_NegDeltaMouse;

    std::unique_ptr<VulkanAPI> m_graphics_api;

    struct ImGuiIO*          m_imgui_io;
    vk::UniqueDescriptorPool m_imguiDescriptorPool;

    /*
     *
     * Minecraft
     *
     * */
    std::unique_ptr<Minecraft> m_MinecraftInstance;

    void InitWindow( );
    void InitImgui( );
    void RecreateWindow( bool isFullScreen );
    void cleanUp( );

    std::atomic<bool> m_render_thread_should_run;
    void              renderThread( );
    void              renderImgui( );

    static void onFrameBufferResized( GLFWwindow* window, int width, int height );
    static void onKeyboardInput( GLFWwindow* window, int key, int scancode, int action, int mods );
    static void onMousePositionInput( GLFWwindow* window, double xpos, double ypos );

public:
    MainApplication( );
    ~MainApplication( );

    inline const std::pair<double, double>& GetDeltaMouse( )
    {
        return m_NegDeltaMouse;
    }

    std::pair<float, float> GetMovementDelta( );

    void LockMouse( )
    {
        m_is_mouse_locked = true;
        glfwSetInputMode( m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
    }
    
    void UnlockMouse( )
    {
        m_is_mouse_locked = false;
        glfwSetInputMode( m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
    }

    [[nodiscard]] VulkanAPI& GetVulkanAPI( ) const { return *m_graphics_api.get( ); }

    void run( );
};

#endif   // MINECRAFT_VK_VULKAN_MAINAPPLICATION_HPP
