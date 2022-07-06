//
// Created by loys on 16/1/2022.
//

#include <Include/GLM.hpp>
#include <Include/imgui_include.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <Include/implot/implot.h>
#include <Include/implot/implot_internal.h>

#include "MainApplication.hpp"

#include "Include/GlobalConfig.hpp"
#include "Include/GraphicAPI.hpp"
#include "Utility/Timer.hpp"

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

    m_graphics_api->addDesiredQueueType( &m_imgui_io, vk::QueueFlagBits::eGraphics );
    m_graphics_api->setupAPI( "Minecraft" );

    InitImgui( );

    m_MinecraftInstance = std::make_unique<Minecraft>( );
    m_MinecraftInstance->InitServer( );

    Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Finished Initializing" );
}

MainApplication::~MainApplication( )
{
    cleanUp( );
}

void
MainApplication::run( )
{
    const auto                           swapChainImagesCount = m_graphics_api->getSwapChainImagesCount( );
    std::vector<VulkanAPI::VKBufferMeta> uniformBuffers( swapChainImagesCount );
    const auto                           updateDescriptorSet = [ this, swapChainImagesCount, &uniformBuffers ]( ) {
        for ( size_t i = 0; i < swapChainImagesCount; i++ )
        {
            vk::DescriptorBufferInfo bufferInfo;
            bufferInfo.setBuffer( *uniformBuffers[ i ].buffer )
                .setOffset( 0 )
                .setRange( sizeof( TransformUniformBufferObject ) );

            m_graphics_api->getLogicalDevice( )
                .updateDescriptorSets( m_graphics_api->getWriteDescriptorSetSetup( i )
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

    auto ubos = std::make_unique<TransformUniformBufferObject[]>( swapChainImagesCount );
    for ( int i = 0; i < swapChainImagesCount; ++i )
    {
        ubos[ i ].model = glm::mat4( 1.0f );
        ubos[ i ].proj  = glm::perspective( glm::radians( 104 / 2.f ), m_graphics_api->getDisplayExtent( ).width / (float) m_graphics_api->getDisplayExtent( ).height, 0.1f, 500.0f );
    }

    std::vector<VulkanAPI::VKMBufferMeta> indirectDrawBuffers( swapChainImagesCount, m_graphics_api->getMemoryAllocator( ) );

    m_graphics_api->setRenderer( [ &uniformBuffers, &ubos, &indirectDrawBuffers, this ]( const vk::CommandBuffer& command_buffer, uint32_t index ) {
        static auto startTime = std::chrono::high_resolution_clock::now( );

        if ( m_screen_width * m_screen_height == 0 )
            return;   // window minimized, not render

        const auto  currentTime = std::chrono::high_resolution_clock::now( );
        const float time        = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count( );

        ubos[ index ].view = MinecraftServer::GetInstance( ).GetPlayer( 0 ).GetViewMatrix( );
        ubos[ index ].proj = glm::perspective( glm::radians( 104 / 2.f ), m_graphics_api->getDisplayExtent( ).width / (float) m_graphics_api->getDisplayExtent( ).height, 0.1f, 5000.0f );

        // for vulkan coordinate system
        ubos[ index ].proj[ 1 ][ 1 ] *= -1;
        uniformBuffers[ index ].writeBuffer( &ubos[ index ], sizeof( TransformUniformBufferObject ), *m_graphics_api );
        command_buffer.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, m_graphics_api->getPipelineLayout( ), 0, m_graphics_api->getDescriptorSets( )[ index ], nullptr );
        auto& chunkPool = MinecraftServer::GetInstance( ).GetWorld( ).GetChunkPool( );

        m_renderingChunkCount = 0;

        {
            std::lock_guard renderBufferLock( chunkPool.GetRenderBufferLock( ) );
            auto&           renderBuffer = chunkPool.GetRenderBuffer( );

            for ( auto& buffer : renderBuffer.m_Buffers )
            {
                if ( buffer.m_DataSlots.empty( ) || !buffer.indirectDrawBuffers.GetBuffer( ) ) continue;

                std::lock_guard<std::mutex> indirectDrawBufferLock( buffer.indirectDrawBuffersMutex );
                command_buffer.bindVertexBuffers( 0, buffer.vertexBuffer, vk::DeviceSize( 0 ) );

                static_assert( std::is_same<IndexBufferType, uint32_t>::value );
                command_buffer.bindIndexBuffer( buffer.indexBuffer, 0, vk::IndexType::eUint32 );

                command_buffer.drawIndexedIndirect( buffer.indirectDrawBuffers.GetBuffer( ), 0, buffer.indirectCommands.size( ), sizeof( vk::DrawIndexedIndirectCommand ) );
            }
        }


        ImGui_ImplVulkan_NewFrame( );
        ImGui_ImplGlfw_NewFrame( );
        ImGui::NewFrame( );

        renderImgui( );

        // Rendering
        ImGui::Render( );
        ImDrawData* draw_data = ImGui::GetDrawData( );
        ImGui_ImplVulkan_RenderDrawData( draw_data, command_buffer );
    } );
    m_graphics_api->cycleGraphicCommandBuffers( );

    /*
     *
     * Start game loop
     *
     * */

    {
        std::jthread render_thread( std::bind_front( &MainApplication::renderThread, this ) );
        while ( !glfwWindowShouldClose( m_window ) )
        {
            glfwPollEvents( );
        }
    }

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
    glfwSetCursorPosCallback( m_window, onMousePositionInput );
    glfwSetKeyCallback( m_window, onKeyboardInput );

    LockMouse( );
}

void
MainApplication::InitImgui( )
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION( );
    ImGui::CreateContext( );
    ImPlot::CreateContext( );
    m_imgui_io = &ImGui::GetIO( );
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark( );
    // ImGui::StyleColorsClassic();


    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan( m_window, true );
    ImGui_ImplVulkan_InitInfo init_info = { };
    init_info.Instance                  = m_graphics_api->getVulkanInstance( );
    init_info.PhysicalDevice            = m_graphics_api->getPhysicalDevice( );
    init_info.Device                    = m_graphics_api->getLogicalDevice( );

    const auto& imguiQueueFamily = m_graphics_api->getDesiredQueueIndices( &m_imgui_io );
    init_info.QueueFamily        = imguiQueueFamily.first;
    init_info.Queue              = m_graphics_api->getLogicalDevice( ).getQueue( imguiQueueFamily.first, imguiQueueFamily.second );

    // TODO: we need this later?
    init_info.PipelineCache = nullptr;

    // Create Descriptor Pool
    {
        VkDescriptorPoolSize pool_sizes[] =
            {
                {               VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                {         VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                {         VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                {  VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                {  VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                {        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                {        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                {      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
        };
        VkDescriptorPoolCreateInfo pool_info = { };
        pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets                    = 1000 * IM_ARRAYSIZE( pool_sizes );
        pool_info.poolSizeCount              = (uint32_t) IM_ARRAYSIZE( pool_sizes );
        pool_info.pPoolSizes                 = pool_sizes;
        m_imguiDescriptorPool                = m_graphics_api->getLogicalDevice( ).createDescriptorPoolUnique( pool_info );
    }

    init_info.DescriptorPool  = *m_imguiDescriptorPool;
    init_info.Subpass         = 0;
    init_info.MinImageCount   = m_graphics_api->getSwapChainImagesCount( );
    init_info.ImageCount      = m_graphics_api->getSwapChainImagesCount( );
    init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator       = nullptr;
    init_info.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init( &init_info, m_graphics_api->getRenderPass( ) );

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    // IM_ASSERT(font != NULL);

    // Upload Fonts
    {
        // Use any command queue
        auto                          command_pool = m_graphics_api->getLogicalDevice( ).createCommandPoolUnique( { { vk::CommandPoolCreateFlagBits::eResetCommandBuffer }, imguiQueueFamily.first } );
        vk::CommandBufferAllocateInfo allocInfo { };
        allocInfo.setCommandPool( command_pool.get( ) );
        allocInfo.setLevel( vk::CommandBufferLevel::ePrimary );
        allocInfo.setCommandBufferCount( 1 );

        auto command_buffer = m_graphics_api->getLogicalDevice( ).allocateCommandBuffers( allocInfo );

        vkResetCommandPool( m_graphics_api->getLogicalDevice( ), *command_pool, 0 );
        VkCommandBufferBeginInfo begin_info = { };
        begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer( command_buffer[ 0 ], &begin_info );

        ImGui_ImplVulkan_CreateFontsTexture( command_buffer[ 0 ] );

        VkSubmitInfo end_info       = { };
        end_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers    = (VkCommandBuffer*) &command_buffer[ 0 ];
        vkEndCommandBuffer( command_buffer[ 0 ] );
        vkQueueSubmit( init_info.Queue, 1, &end_info, VK_NULL_HANDLE );

        m_graphics_api->getLogicalDevice( ).waitIdle( );
        ImGui_ImplVulkan_DestroyFontUploadObjects( );
    }

    ImGuiStyle& style       = ImGui::GetStyle( );
    style.WindowRounding    = 5.3f;
    style.FrameRounding     = 2.3f;
    style.ScrollbarRounding = 0;

    style.Colors[ ImGuiCol_Text ]                 = ImVec4( 0.00f, 0.00f, 0.00f, 1.00f );
    style.Colors[ ImGuiCol_TextDisabled ]         = ImVec4( 0.60f, 0.60f, 0.60f, 1.00f );
    style.Colors[ ImGuiCol_WindowBg ]             = ImVec4( 0.94f, 0.94f, 0.94f, 1.00f );
    style.Colors[ ImGuiCol_Border ]               = ImVec4( 0.00f, 0.00f, 0.00f, 0.39f );
    style.Colors[ ImGuiCol_BorderShadow ]         = ImVec4( 1.00f, 1.00f, 1.00f, 0.10f );
    style.Colors[ ImGuiCol_FrameBg ]              = ImVec4( 1.00f, 1.00f, 1.00f, 1.00f );
    style.Colors[ ImGuiCol_FrameBgHovered ]       = ImVec4( 0.26f, 0.59f, 0.98f, 0.40f );
    style.Colors[ ImGuiCol_FrameBgActive ]        = ImVec4( 0.26f, 0.59f, 0.98f, 0.67f );
    style.Colors[ ImGuiCol_TitleBg ]              = ImVec4( 0.96f, 0.96f, 0.96f, 1.00f );
    style.Colors[ ImGuiCol_TitleBgCollapsed ]     = ImVec4( 1.00f, 1.00f, 1.00f, 0.51f );
    style.Colors[ ImGuiCol_TitleBgActive ]        = ImVec4( 0.82f, 0.82f, 0.82f, 1.00f );
    style.Colors[ ImGuiCol_MenuBarBg ]            = ImVec4( 0.86f, 0.86f, 0.86f, 1.00f );
    style.Colors[ ImGuiCol_ScrollbarBg ]          = ImVec4( 0.98f, 0.98f, 0.98f, 0.53f );
    style.Colors[ ImGuiCol_ScrollbarGrab ]        = ImVec4( 0.69f, 0.69f, 0.69f, 0.80f );
    style.Colors[ ImGuiCol_ScrollbarGrabHovered ] = ImVec4( 0.49f, 0.49f, 0.49f, 0.80f );
    style.Colors[ ImGuiCol_ScrollbarGrabActive ]  = ImVec4( 0.49f, 0.49f, 0.49f, 1.00f );
    style.Colors[ ImGuiCol_SliderGrab ]           = ImVec4( 0.26f, 0.59f, 0.98f, 0.78f );
    style.Colors[ ImGuiCol_SliderGrabActive ]     = ImVec4( 0.26f, 0.59f, 0.98f, 1.00f );
    style.Colors[ ImGuiCol_Button ]               = ImVec4( 0.26f, 0.59f, 0.98f, 0.40f );
    style.Colors[ ImGuiCol_ButtonHovered ]        = ImVec4( 0.26f, 0.59f, 0.98f, 1.00f );
    style.Colors[ ImGuiCol_ButtonActive ]         = ImVec4( 0.06f, 0.53f, 0.98f, 1.00f );
    style.Colors[ ImGuiCol_Header ]               = ImVec4( 0.26f, 0.59f, 0.98f, 0.31f );
    style.Colors[ ImGuiCol_HeaderHovered ]        = ImVec4( 0.26f, 0.59f, 0.98f, 0.80f );
    style.Colors[ ImGuiCol_HeaderActive ]         = ImVec4( 0.26f, 0.59f, 0.98f, 1.00f );
    style.Colors[ ImGuiCol_ResizeGripHovered ]    = ImVec4( 0.26f, 0.59f, 0.98f, 0.67f );
    style.Colors[ ImGuiCol_ResizeGripActive ]     = ImVec4( 0.26f, 0.59f, 0.98f, 0.95f );
    style.Colors[ ImGuiCol_PlotLinesHovered ]     = ImVec4( 1.00f, 0.43f, 0.35f, 1.00f );
    style.Colors[ ImGuiCol_PlotHistogram ]        = ImVec4( 0.90f, 0.70f, 0.00f, 1.00f );
    style.Colors[ ImGuiCol_PlotHistogramHovered ] = ImVec4( 1.00f, 0.60f, 0.00f, 1.00f );
    style.Colors[ ImGuiCol_TextSelectedBg ]       = ImVec4( 0.26f, 0.59f, 0.98f, 0.35f );
}

void
MainApplication::cleanUp( )
{
    glfwDestroyWindow( m_window );
    glfwTerminate( );
}

void
MainApplication::renderThread( const std::stop_token& st )
{
    while ( !st.stop_requested( ) )
    {

        m_deltaMouseHoldUpdate.test_and_set( );

        /*
         *
         * Update game loop
         *
         * */
        m_MinecraftInstance->Tick( m_imgui_io->DeltaTime );

        /*
         *
         * Reset delta input
         *
         * */
        m_NegDeltaMouse = { };
        m_deltaMouseHoldUpdate.clear( );

        /*
         *
         * Render
         *
         * */

        const uint32_t image_index = m_graphics_api->acquireNextImage( );
        // m_graphics_api->cycleGraphicCommandBuffers( image_index );
        m_graphics_api->presentFrame<true>( image_index );

        // m_graphics_api->waitPresent( );
    }

    m_graphics_api->getLogicalDevice( ).waitIdle( );

    ImGui_ImplVulkan_Shutdown( );
    ImGui_ImplGlfw_Shutdown( );
    ImPlot::DestroyContext( );
    ImGui::DestroyContext( );
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
        glfwSetWindowMonitor( m_window, glfwGetPrimaryMonitor( ), 0, 0, mode->width, mode->height, GLFW_DONT_CARE );
    } else
    {
        glfwSetWindowMonitor( m_window, nullptr, m_screen_pos_x, m_screen_pos_y, m_backup_screen_width, m_backup_screen_height, GLFW_DONT_CARE );
    }

    m_window_fullscreen = isFullScreen;
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
            app->UnlockMouse( );
            // glfwSetWindowShouldClose( app->m_window, true );
        }
        break;
    case GLFW_RELEASE:
    case GLFW_REPEAT:
    default:

        break;
    }
}

// utility structure for realtime plot
struct ScrollingBuffer {
    int              MaxSize;
    int              Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer( int max_size = 2000 )
    {
        MaxSize = max_size;
        Offset  = 0;
        Data.reserve( MaxSize );
    }
    void AddPoint( float x, float y )
    {
        if ( Data.size( ) < MaxSize )
            Data.push_back( ImVec2( x, y ) );
        else
        {
            Data[ Offset ] = ImVec2( x, y );
            Offset         = ( Offset + 1 ) % MaxSize;
        }
    }
    void Erase( )
    {
        if ( Data.size( ) > 0 )
        {
            Data.shrink( 0 );
            Offset = 0;
        }
    }
};

void
MainApplication::renderImgui( )
{
    static bool   show_demo_window    = false;
    static bool   show_another_window = false;
    static ImVec4 clear_color         = ImVec4( 0.0515186f, 0.504163f, 0.656863f, 1.0f );

    if ( m_is_mouse_locked ) return;

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if ( show_demo_window )
        ImGui::ShowDemoWindow( &show_demo_window );

    // 2. Show a simple window that weF create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f       = 0.0f;
        static int   counter = 0;

        ImGui::Begin( "Debuger" );                             // Create a window called "Hello, world!" and append into it.
        ImGui::Checkbox( "Demo Window", &show_demo_window );   // Edit bools storing our window open/close state
        ImGui::Checkbox( "Another Window", &show_another_window );

        ImGui::SliderFloat( "float", &f, 0.0f, 1.0f );                     // Edit 1 float using a slider from 0.0f to 1.0f
        if ( ImGui::ColorEdit3( "clear color", (float*) &clear_color ) )   // Edit 3 floats representing a color
        {
            Logger::getInstance( ).LogLine( "Background", std::make_tuple( clear_color.x, clear_color.y, clear_color.z, 1.0f ) );
            m_graphics_api->setClearColor( { clear_color.x, clear_color.y, clear_color.z, 1.0f } );
        }

        if ( ImGui::Button( "Fps mode" ) )
        {
            LockMouse( );
        }

        ImGui::SameLine( );
        if ( ImGui::Button( "Sleep 1s" ) )
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for( 1s );
        }

        ImGui::Text( "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / m_imgui_io->Framerate, m_imgui_io->Framerate );

        {
            static float            t         = 0;
            static float            previousT = 0;
            static float            history   = 10.0f;
            static ImVector<ImVec2> chunkLoadingThreadCount, chunkCount, chunkRenderCount;
            auto&                   chunkPool = MinecraftServer::GetInstance( ).GetWorld( ).GetChunkPool( );

            t += ImGui::GetIO( ).DeltaTime;
            ImGui::SliderFloat( "History", &history, 1, 30, "%.1f s" );

            if ( chunkLoadingThreadCount.empty( ) || t > previousT + 0.25 )
            {
                previousT  = t;
                float xmod = fmodf( t, history );
                if ( !chunkLoadingThreadCount.empty( ) && xmod < chunkLoadingThreadCount.back( ).x )
                {
                    chunkLoadingThreadCount.shrink( 0 );
                    chunkCount.shrink( 0 );
                    chunkRenderCount.shrink( 0 );
                }
                chunkLoadingThreadCount.push_back( ImVec2( xmod, chunkPool.GetLoadingCount( ) ) );
                chunkCount.push_back( ImVec2( xmod, chunkPool.GetTotalChunk( ) ) );
                chunkRenderCount.push_back( ImVec2( xmod, m_renderingChunkCount ) );
            }

            static ScrollingBuffer fps;
            fps.AddPoint( t, m_imgui_io->Framerate );

            if ( ImPlot::BeginPlot( "FPS##Scrolling", ImVec2( -1, 150 ) ) )
            {
                ImPlot::SetupAxes( nullptr, nullptr, ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit );
                ImPlot::SetupAxisLimits( ImAxis_X1, std::max( fps.Data[ fps.Offset ].x, t - history ), t, ImGuiCond_Always );
                ImPlot::SetNextFillStyle( IMPLOT_AUTO_COL, 0.5f );
                ImPlot::PlotShaded( "", &fps.Data[ 0 ].x, &fps.Data[ 0 ].y, fps.Data.size( ), -INFINITY, 0, fps.Offset, 2 * sizeof( float ) );
                ImPlot::EndPlot( );
            }

            if ( ImPlot::BeginPlot( "Chunks##Scrolling", ImVec2( -1, 0 ) ) )
            {
                ImPlot::SetupAxes( "Time", "Thread" );
                ImPlot::SetupAxis( ImAxis_Y2, "Chunk", ImPlotAxisFlags_AuxDefault | ImPlotAxisFlags_AutoFit );
                ImPlot::SetupAxisLimits( ImAxis_X1, -0.5, history + 0.5, ImGuiCond_Always );
                ImPlot::SetupAxisLimits( ImAxis_Y1, 0, chunkPool.GetMaxThread( ) + 1 );

                ImPlot::SetAxes( ImAxis_X1, ImAxis_Y1 );
                ImPlot::SetNextMarkerStyle( ImPlotMarker_Asterisk );
                ImPlot::PlotStems( "Chunk thread", &chunkLoadingThreadCount[ 0 ].x, &chunkLoadingThreadCount[ 0 ].y, chunkLoadingThreadCount.size( ), 0, 0, 0, 2 * sizeof( float ) );

                ImPlot::SetAxes( ImAxis_X1, ImAxis_Y2 );
                ImPlot::SetNextMarkerStyle( ImPlotMarker_Circle );
                ImPlot::PlotStems( "Total Chunk", &chunkCount[ 0 ].x, &chunkCount[ 0 ].y, chunkCount.size( ), 0, 0, 0, 2 * sizeof( float ) );
                ImPlot::SetNextMarkerStyle( ImPlotMarker_Diamond );
                ImPlot::PlotStems( "Total Chunk Rendering", &chunkRenderCount[ 0 ].x, &chunkRenderCount[ 0 ].y, chunkRenderCount.size( ), 0, 0, 0, 2 * sizeof( float ) );
                ImPlot::EndPlot( );
            }
        }

        ImGui::End( );
    }

    // 3. Show another simple window.
    if ( show_another_window )
    {
        ImPlot::ShowDemoWindow( &show_another_window );
    }
}

void
MainApplication::onMousePositionInput( GLFWwindow* window, double xpos, double ypos )
{
    auto* mainApplication = reinterpret_cast<MainApplication*>( glfwGetWindowUserPointer( window ) );

    if ( !mainApplication->m_deltaMouseHoldUpdate.test( ) && mainApplication->m_is_mouse_locked )
        mainApplication->m_NegDeltaMouse = { xpos - mainApplication->m_MousePos.first, mainApplication->m_MousePos.second - ypos };
    mainApplication->m_MousePos = { xpos, ypos };

    /*
        Logger::getInstance( ).LogLine( xpos, ypos );
        auto mousePos = reinterpret_cast<MainApplication*>( glfwGetWindowUserPointer( window ) )->m_imgui_io->MousePos;
        Logger::getInstance( ).LogLine( mousePos.x, mousePos.y );
    */
}

std::pair<float, float>
MainApplication::GetMovementDelta( )
{
    std::pair<float, float> offset;
    if ( glfwGetKey( m_window, GLFW_KEY_A ) == GLFW_PRESS ) offset.first = -1;
    if ( glfwGetKey( m_window, GLFW_KEY_D ) == GLFW_PRESS ) offset.first += 1;
    if ( glfwGetKey( m_window, GLFW_KEY_S ) == GLFW_PRESS ) offset.second = -1;
    if ( glfwGetKey( m_window, GLFW_KEY_W ) == GLFW_PRESS ) offset.second += 1;

    return offset;
}
