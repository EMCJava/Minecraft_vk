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

    auto& logicalDevice = m_graphics_api->getLogicalDevice( );

    auto vertex_buffer   = logicalDevice.createBufferUnique( { { }, size_of_data, vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive } );
    auto memRequirements = logicalDevice.getBufferMemoryRequirements( vertex_buffer.get( ) );
    auto memProperties   = m_graphics_api->getPhysicalDevice( ).getMemoryProperties( );

    auto findMemoryType = [ &memProperties ]( uint32_t typeFilter, vk::MemoryPropertyFlags properties ) -> uint32_t {
        for ( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ )
            if ( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[ i ].propertyFlags & properties ) == properties ) return i;
        throw std::runtime_error( "failed to find suitable memory type!" );
    };

    auto vertex_buffer_memory = logicalDevice.allocateMemoryUnique( { memRequirements.size, findMemoryType( memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent ) } );
    logicalDevice.bindBufferMemory( vertex_buffer.get( ), vertex_buffer_memory.get( ), 0 );

    memcpy( logicalDevice.mapMemory( vertex_buffer_memory.get( ), 0, size_of_data ), vertices.data( ), (size_t) size_of_data );
    logicalDevice.unmapMemory( vertex_buffer_memory.get( ) );

    m_graphics_api->setRenderer( [ &vertex_buffer ]( const vk::CommandBuffer& command_buffer ) {
        vk::Buffer     vertexBuffers[] = { vertex_buffer.get() };
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
    auto* app = reinterpret_cast<MainApplication*>( glfwGetWindowUserPointer( window ) );
    assert( window == app->m_window );

    app->m_graphics_api->setShouldCreateSwapChain( width * height );
    app->m_graphics_api->invalidateSwapChain( );
    std::tie( app->m_screen_width, app->m_screen_height ) = { width, height };
}
