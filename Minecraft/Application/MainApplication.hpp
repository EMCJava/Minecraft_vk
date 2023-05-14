//
// Created by loys on 16/1/2022.
//

#ifndef MINECRAFT_VK_VULKAN_MAINAPPLICATION_HPP
#define MINECRAFT_VK_VULKAN_MAINAPPLICATION_HPP


#include <Graphic/Vulkan/VulkanAPI.hpp>
#include <Minecraft/Application/Input/UserInput.hpp>
#include <Minecraft/Minecraft.hpp>
#include <Minecraft/World/Chunk/RenderableChunk.hpp>
#include <Utility/ImguiAddons/CurveEditor.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>

class MainApplication : public Singleton<MainApplication>
{
    std::unique_ptr<VulkanAPI> m_graphics_api;

    GLFWwindow* m_window { };
    int         m_backup_screen_width { }, m_backup_screen_height { };
    int         m_screen_width { }, m_screen_height { };
    int         m_screen_pos_x { }, m_screen_pos_y { };
    bool        m_window_resizable  = false;
    bool        m_window_fullscreen = false;
    bool        m_is_mouse_locked   = false;

    UserInput                   m_UserInput;
    std::pair<FloatTy, FloatTy> m_MousePos { };

    std::atomic_flag            m_deltaMouseHoldUpdate { };
    std::pair<FloatTy, FloatTy> m_NegDeltaMouse { };

    struct ImGuiIO*          m_imgui_io { };
    vk::UniqueDescriptorPool m_imguiDescriptorPool;

    /*
     *
     * Render
     *
     * */

    struct BlockTransformUBO {
        glm::mat4 view { };
        glm::mat4 proj { };
        alignas( 16 ) glm::vec3 highlightCoordinate { };
        float time = 0;
    };

    struct UBOData {
        BlockTransformUBO ubo;
        float             previousFOV { };
    };

    std::unique_ptr<UBOData[]> renderUBOs;

    std::unique_ptr<ChunkSolidBuffer> m_ChunkSolidBuffers;

    /*
     *
     * Minecraft
     *
     * */
    void                     SetGenerationOffsetByCurve( );
    ImGuiAddons::CurveEditor m_TerrainNoiseOffset {
        {{ 0.f, 0.f }, { 1.f, 0.f }}
    };

    std::unique_ptr<Minecraft> m_MinecraftInstance;
    bool                       m_ShouldReset = false;

    std::mutex                                       m_BlockDetailLock;
    std::vector<std::pair<std::string, std::string>> m_BlockDetailMap;

    /*
     *
     * Statistics
     *
     * */
    uint32_t m_renderingChunkCount = 0;

    void InitWindow( );
    void InitImgui( );
    void RecreateWindow( bool isFullScreen );
    void cleanUp( );

    void renderThread( const std::stop_token& st );
    void renderImgui( uint32_t renderIndex );
    void renderImguiCursor( uint32_t renderIndex ) const;

    void RenderThreadMouseHandle( );

    static void onFrameBufferResized( GLFWwindow* window, int width, int height );
    static void onKeyboardInput( GLFWwindow* window, int key, int scancode, int action, int mods );
    static void onMousePositionInput( GLFWwindow* window, double xpos, double ypos );

public:
    MainApplication( );
    ~MainApplication( );

    inline const std::pair<FloatTy, FloatTy>& GetDeltaMouse( )
    {
        return m_NegDeltaMouse;
    }

    std::pair<float, float> GetMovementDelta( );

    inline auto IsJumping( ) const
    {
        return glfwGetKey( m_window, GLFW_KEY_SPACE ) == GLFW_PRESS;
    }

    inline auto IsCrouch( ) const
    {
        return glfwGetKey( m_window, GLFW_KEY_LEFT_SHIFT ) == GLFW_PRESS;
    }

    inline auto IsSprint( ) const
    {
        return glfwGetKey( m_window, GLFW_KEY_LEFT_CONTROL ) == GLFW_PRESS;
    }

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
