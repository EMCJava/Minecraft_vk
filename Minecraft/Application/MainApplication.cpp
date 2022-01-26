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

    Logger::getInstance( ).LogLine( "Finished Initializing" );
}

MainApplication::~MainApplication( )
{
    cleanUp( );
}

void
MainApplication::run( )
{
    m_graphics_api->setRenderer( []( const auto& command_buffer ) { command_buffer.draw( 3, 1, 0, 0 ); } );
    m_graphics_api->cycleGraphicCommandBuffers( );

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
            Logger::getInstance( ).LogLine( "Time used:", ( 1000.f * output_per_frame ) / std::chrono::duration_cast<std::chrono::milliseconds>( time_used ).count( ), "milliseconds" );
            start_time = std::chrono::high_resolution_clock::now( );
        }

        const uint32_t image_index = m_graphics_api->acquireNextImage( );
        m_graphics_api->presentFrame( image_index );
        // m_graphics_api->waitPresent( );
    }
}

void
MainApplication::onFrameBufferResized( GLFWwindow* window, int width, int height )
{
    auto app = reinterpret_cast<MainApplication*>( glfwGetWindowUserPointer( window ) );
    assert( window == app->m_window );

    app->m_graphics_api->setShouldCreateSwapChain( width * height );
    app->m_graphics_api->invalidateSwapChain( );
    std::tie( app->m_screen_width, app->m_screen_height ) = { width, height };
}
