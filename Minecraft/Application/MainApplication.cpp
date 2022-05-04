//
// Created by loys on 16/1/2022.
//

#include "MainApplication.hpp"

#include "Include/GlobalConfig.hpp"
#include "Include/GraphicAPI.hpp"

#include <chrono>
#include <thread>
#include <unordered_set>

MainApplication::MainApplication( )
{
    InitWindow( );

    m_graphics_api = std::make_unique<VulkanAPI>( m_window );
    m_graphics_api->setupAPI( "Minecraft" );

    Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Finished Initializing" );
}

MainApplication::~MainApplication( )
{
    cleanUp( );
}

void
MainApplication::run( )
{
    /*
     *
     * Setup vertex buffer
     *
     * */
    const std::vector<DataType::ColoredVertex> vertices = {
        {{ 0.0f, -0.5f }, { 1.0f, 1.0f, 1.0f }},
        { { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }},
        {{ -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }}
    };
    const auto size_of_data = sizeof( vertices[ 0 ] ) * vertices.size( );

    VulkanAPI::VKBufferMeta vertexBuffer;
    vertexBuffer.Create( size_of_data, vk::BufferUsageFlagBits::eVertexBuffer, *m_graphics_api );
    vertexBuffer.BindBuffer( vertices.data( ), size_of_data, *m_graphics_api );

    m_graphics_api->setRenderer( [ &vertexBuffer ]( const vk::CommandBuffer& command_buffer ) {
        vk::Buffer     vertexBuffers[] = { vertexBuffer.buffer.get() };
        vk::DeviceSize offsets[]       = { 0 };
        command_buffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
        command_buffer.draw( 3, 1, 0, 0 ); } );
    m_graphics_api->cycleGraphicCommandBuffers( );

    /*
     *
     * Start game loop
     *
     * */
    m_render_thread_should_run = true;
    std::thread render_thread( &MainApplication::renderThread, this );
    while ( !glfwWindowShouldClose( m_window ) )
    {
        glfwPollEvents( );
    }

    m_render_thread_should_run = false;
    if ( render_thread.joinable( ) ) render_thread.join( );
    m_graphics_api->waitIdle( );
}

void
MainApplication::InitWindow( )
{
    glfwInit( );
    glfwSetErrorCallback( ValidationLayer::glfwErrorCallback );

    m_screen_width     = GlobalConfig::getConfigData( )[ "screen_width" ].get<int>( );
    m_screen_height    = GlobalConfig::getConfigData( )[ "screen_height" ].get<int>( );
    m_window_resizable = GlobalConfig::getConfigData( )[ "window_resizable" ].get<bool>( );

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, m_window_resizable ? GLFW_TRUE : GLFW_FALSE );
    m_window = glfwCreateWindow( m_screen_width, m_screen_height, "Vulkan window", nullptr, nullptr );
    glfwSetWindowUserPointer( m_window, this );
    glfwSetFramebufferSizeCallback( m_window, onFrameBufferResized );
    glfwSetKeyCallback( m_window, onKeyboardInput );
}

void
MainApplication::cleanUp( )
{
    glfwDestroyWindow( m_window );
    glfwTerminate( );
}

void
MainApplication::renderThread( )
{

    constexpr uint32_t output_per_frame = 2500 * 2;
    uint32_t           frame_count      = 0;
    auto               start_time       = std::chrono::high_resolution_clock::now( );
    while ( m_render_thread_should_run )
    {

        ++frame_count;
        if ( frame_count % output_per_frame == 0 )
        {
            auto time_used = std::chrono::high_resolution_clock::now( ) - start_time;
            Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "fps:", ( 1000.f * output_per_frame ) / std::chrono::duration_cast<std::chrono::milliseconds>( time_used ).count( ) );
            start_time = std::chrono::high_resolution_clock::now( );
        }

        const uint32_t image_index = m_graphics_api->acquireNextImage( );
        // m_graphics_api->cycleGraphicCommandBuffers( image_index );
        m_graphics_api->presentFrame<true>( image_index );
        // m_graphics_api->waitPresent( );
    }
}

void
MainApplication::onFrameBufferResized( GLFWwindow* window, int width, int height )
{
    static auto* app = reinterpret_cast<MainApplication*>( glfwGetWindowUserPointer( window ) );
    assert( window == app->m_window );

    // if width or height is zero, window is minimized
    app->m_graphics_api->setShouldCreateSwapChain( width * height );
    app->m_graphics_api->invalidateSwapChain( );
    std::tie( app->m_screen_width, app->m_screen_height ) = { width, height };
}

void
MainApplication::RecreateWindow( bool isFullScreen )
{
    const bool isCurrentFullScreen = glfwGetWindowMonitor( m_window ) != nullptr;

    if ( isCurrentFullScreen == isFullScreen )
    {
        Logger::getInstance( ).LogLine( Logger::LogType::eWarn, "Setting fullscreen(", isFullScreen, "), stays the same" );
        return;
    }

    if ( isFullScreen )
    {
        // backup window position and window size
        glfwGetWindowPos( m_window, &m_screen_pos_x, &m_screen_pos_y );
        glfwGetWindowSize( m_window, &m_backup_screen_width, &m_backup_screen_height );

        // get resolution of monitor
        const GLFWvidmode* mode = glfwGetVideoMode( glfwGetPrimaryMonitor( ) );

        // switch to full screen
        glfwSetWindowMonitor( m_window, glfwGetPrimaryMonitor( ), 0, 0, mode->width, mode->height, 0 );
    } else
    {
        glfwSetWindowMonitor( m_window, nullptr, m_screen_pos_x, m_screen_pos_y, m_backup_screen_width, m_backup_screen_height, 0 );
    }

    m_graphics_api->setShouldCreateSwapChain( true );
    m_graphics_api->invalidateSwapChain( );
}

void
MainApplication::onKeyboardInput( GLFWwindow* window, int key, int scancode, int action, int mods )
{
    static auto* app = reinterpret_cast<MainApplication*>( glfwGetWindowUserPointer( window ) );

    switch ( action )
    {
    case GLFW_PRESS:
        if ( key == GLFW_KEY_F11 )
        {
            // toggle fullscreen
            app->RecreateWindow( glfwGetWindowMonitor( app->m_window ) == nullptr );
        }
        break;
    case GLFW_RELEASE:
    case GLFW_REPEAT:
    default:

        break;
    }
}
