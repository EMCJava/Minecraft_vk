//
// Created by loys on 16/1/2022.
//

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include "MainApplication.hpp"

#include "Include/GlobalConfig.hpp"
#include "Include/GraphicAPI.hpp"

#include <chrono>
#include <thread>
#include <unordered_set>

struct TransformUniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

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
        {{ -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
        { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }},
        {  { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }},
        { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f }}
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0 };

    const auto verticesDataSize = sizeof( vertices[ 0 ] ) * vertices.size( );
    const auto indicesDataSize  = sizeof( indices[ 0 ] ) * indices.size( );

    VulkanAPI::VKBufferMeta vertexBuffer, indexBuffer;

    {
        VulkanAPI::VKBufferMeta stagingBuffer;
        vk::BufferCopy          bufferRegion;
        using Usage = vk::BufferUsageFlagBits;

        stagingBuffer.Create( verticesDataSize, Usage::eVertexBuffer | Usage::eTransferSrc, *m_graphics_api );

        stagingBuffer.BindBuffer( vertices.data( ), verticesDataSize, *m_graphics_api );
        vertexBuffer.Create( verticesDataSize, Usage::eVertexBuffer | Usage::eTransferDst, *m_graphics_api,
                             vk::MemoryPropertyFlagBits::eDeviceLocal );

        bufferRegion.setSize( verticesDataSize );
        vertexBuffer.CopyFromBuffer( stagingBuffer, bufferRegion, *m_graphics_api );

        stagingBuffer.BindBuffer( indices.data( ), indicesDataSize, *m_graphics_api );
        indexBuffer.Create( verticesDataSize, Usage::eIndexBuffer | Usage::eTransferDst, *m_graphics_api,
                            vk::MemoryPropertyFlagBits::eDeviceLocal );

        bufferRegion.setSize( indicesDataSize );
        indexBuffer.CopyFromBuffer( stagingBuffer, bufferRegion, *m_graphics_api );
    }

    const auto                           swapChainImagesCount = m_graphics_api->getSwapChainImagesCount( );
    std::vector<VulkanAPI::VKBufferMeta> uniformBuffers( swapChainImagesCount );
    const auto                           updateDescriptorSet = [ this, swapChainImagesCount, &uniformBuffers ]( ) {
        for ( size_t i = 0; i < swapChainImagesCount; i++ )
        {
            vk::DescriptorBufferInfo bufferInfo;
            bufferInfo.setBuffer( *uniformBuffers[ i ].buffer )
                .setOffset( 0 )
                .setRange( sizeof( TransformUniformBufferObject ) );

            m_graphics_api->getLogicalDevice( ).updateDescriptorSets( m_graphics_api->getWriteDescriptorSetSetup( i )
                                                                                                    .setDstBinding( 0 )
                                                                                                    .setDstArrayElement( 0 )
                                                                                                    .setDescriptorType( vk::DescriptorType::eUniformBuffer )
                                                                                                    .setDescriptorCount( 1 )
                                                                                                    .setBufferInfo( bufferInfo ),
                                                                                                nullptr );
        }
    };

    {
        for ( auto& buffer : uniformBuffers )
            buffer.Create( sizeof( TransformUniformBufferObject ), vk::BufferUsageFlagBits::eUniformBuffer, *m_graphics_api );

        updateDescriptorSet( );
        m_graphics_api->setPipelineCreateCallback( updateDescriptorSet );
    }

    m_graphics_api->setRenderer( [ &vertexBuffer, &indexBuffer, &uniformBuffers, this ]( const vk::CommandBuffer& command_buffer, uint32_t index ) {
        static auto startTime = std::chrono::high_resolution_clock::now( );

        auto  currentTime = std::chrono::high_resolution_clock::now( );
        float time        = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count( );

        TransformUniformBufferObject ubo { };
        ubo.model = glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 90.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
        ubo.view  = glm::lookAt( glm::vec3( 2.0f, 2.0f, 2.0f ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
        ubo.proj  = glm::perspective( glm::radians( 45.0f ), m_graphics_api->getDisplayExtent( ).width / (float) m_graphics_api->getDisplayExtent( ).height, 0.1f, 10.0f );

        // for vulkan coordinate system
        ubo.proj[ 1 ][ 1 ] *= -1;
        uniformBuffers[ index ].BindBuffer( &ubo, sizeof( TransformUniformBufferObject ), *m_graphics_api );
        command_buffer.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, m_graphics_api->getPipelineLayout( ), 0, m_graphics_api->getDescriptorSets( )[ index ], nullptr );

        command_buffer.bindVertexBuffers( 0, vertexBuffer.buffer.get( ), vk::DeviceSize( 0 ) );
        command_buffer.bindIndexBuffer( indexBuffer.buffer.get( ), 0, vk::IndexType::eUint16 );

        // command_buffer.draw( 3, 1, 0, 0 );
        command_buffer.drawIndexed( 6, 1, 0, 0, 0 );
    } );
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
    if ( m_window_fullscreen == isFullScreen )
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

    m_window_fullscreen = !m_window_fullscreen;
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
            app->RecreateWindow( !app->m_window_fullscreen );
        } else if ( key == GLFW_KEY_ESCAPE )
        {
            glfwSetWindowShouldClose( app->m_window, true );
        }
        break;
    case GLFW_RELEASE:
    case GLFW_REPEAT:
    default:

        break;
    }
}
