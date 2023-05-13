//
// Created by loys on 1/24/2022.
//

#ifndef MINECRAFT_VK_VULKAN_VULKANAPI_HPP
#define MINECRAFT_VK_VULKAN_VULKANAPI_HPP

#include <Include/GraphicAPI.hpp>
#include <Include/vk_mem_alloc.h>

#include <Graphic/Vulkan/Pipeline/VulkanPipeline.hpp>
#include <Utility/Thread/MutexResources.hpp>
#include <Utility/Vulkan/ValidationLayer.hpp>
#include <Utility/Vulkan/VulkanExtension.hpp>

#include "ImageMeta.hpp"
#include "QueueFamilyManager.hpp"
#include "Utility/Singleton.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace DataType
{

struct TexturedVertex : VertexDetail {
    glm::ivec3 pos;
    glm::vec3  textureCoor_ColorIntensity;
    glm::vec2  accumulatedTextureCoordinate { };


    constexpr TexturedVertex( glm::vec3 p = glm::vec3( 0 ), glm::vec2 c = glm::vec2( 0 ), float i = 1.0f )
        : pos( p )
        , textureCoor_ColorIntensity( c.x, c.y, i )
    { }

    TexturedVertex& operator=( const TexturedVertex& other )
    {
        pos                          = other.pos;
        textureCoor_ColorIntensity   = other.textureCoor_ColorIntensity;
        accumulatedTextureCoordinate = other.accumulatedTextureCoordinate;

        return *this;
    }

    inline void SetTextureCoor( glm::vec2 c )
    {
        textureCoor_ColorIntensity.x = c.x;
        textureCoor_ColorIntensity.y = c.y;
    }

    inline void SetAccumulatedTextureCoor( glm::vec2 c )
    {
        accumulatedTextureCoordinate = c;
    }

    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions( )
    {
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions( 3 );

        attributeDescriptions[ 0 ].setBinding( 0 );
        attributeDescriptions[ 0 ].setLocation( 0 );
        attributeDescriptions[ 0 ].setFormat( vk::Format::eR32G32B32Sint );
        attributeDescriptions[ 0 ].setOffset( offsetof( TexturedVertex, pos ) );

        attributeDescriptions[ 1 ].setBinding( 0 );
        attributeDescriptions[ 1 ].setLocation( 1 );
        attributeDescriptions[ 1 ].setFormat( vk::Format::eR32G32B32Sfloat );
        attributeDescriptions[ 1 ].setOffset( offsetof( TexturedVertex, textureCoor_ColorIntensity ) );

        attributeDescriptions[ 2 ].setBinding( 0 );
        attributeDescriptions[ 2 ].setLocation( 2 );
        attributeDescriptions[ 2 ].setFormat( vk::Format::eR32G32Sfloat );
        attributeDescriptions[ 2 ].setOffset( offsetof( TexturedVertex, accumulatedTextureCoordinate ) );

        return attributeDescriptions;
    }

    static std::vector<vk::VertexInputBindingDescription> getBindingDescriptions( )
    {
        std::vector<vk::VertexInputBindingDescription> bindingDescription( 1 );

        bindingDescription[ 0 ].setBinding( 0 );
        bindingDescription[ 0 ].setStride( sizeof( TexturedVertex ) );
        bindingDescription[ 0 ].setInputRate( vk::VertexInputRate::eVertex );

        return bindingDescription;
    }
};
}   // namespace DataType

class VulkanAPI : public Singleton<VulkanAPI>
{

private:
    struct QueueFamilyIndices {
        std::pair<uint32_t, uint32_t> graphicsFamily;
        uint32_t                      presentFamily;
    };

    struct SwapChainSupportDetails {

        vk::SurfaceCapabilitiesKHR        capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR>   presentModes;

        [[nodiscard]] vk::Extent2D getMaxSwapExtent( GLFWwindow* window ) const;
        [[nodiscard]] bool         isComplete( ) const
        {
            return !formats.empty( ) && !presentModes.empty( );
        }
    };

    /**
     *
     * This function will call setSwapChainSupportDetails
     *
     * */
    bool isDeviceUsable( const vk::PhysicalDevice& device );
    bool isDeviceSupportAllExtensions( const vk::PhysicalDevice& device );
    bool setSwapChainSupportDetails( const vk::PhysicalDevice& device );

    void setupDynamicDispatch( );

    void setupValidationLayer( );
    void setupExtensions( );
    void setupSurface( );

    void selectPhysicalDevice( );
    void setupLogicalDevice( );

    void adeptSwapChainChange( );
    void setupSwapChain( );
    void setupPipeline( );

    void setupVulkanMemoryAllocator( );

    /**
     *
     * Also setup command buffer
     *
     * */
    void setupGraphicCommand( );
    void setupGraphicCommandBuffers( );

    void setupSyncs( );

public:
    explicit VulkanAPI( GLFWwindow* windows );
    ~VulkanAPI( );

    void setupAPI( const std::string& applicationName );

    [[nodiscard]] uint32_t acquireNextImage( );
    void                   setRenderer( std::function<void( const vk::CommandBuffer&, uint32_t index )>&& renderer ) { m_renderer = std::move( renderer ); };
    void                   setPipelineCreateCallback( std::function<void( )>&& callback ) { m_pipeline_create_callback = std::move( callback ); };
    void                   setClearColor( const std::array<float, 4>& clearColor ) { m_clearValues[ 0 ].setColor( clearColor ); }

    /*
     *
     * Run(cycle) renderer on command buffer
     *
     *  @param index Set to -1 to cycle all buffer
     *
     * */
    void cycleGraphicCommandBuffers( uint32_t index = -1 );

    template <bool doRender = false>
    void presentFrame( uint32_t index = -1 );

    void FlushFence( )
    {
        /**
         *
         * Sync in renderer
         *
         * */
        for ( int i = 0; i < m_vkRender_fence_syncs.size( ); ++i )
        {
            auto wait_fence_result = m_vkLogicalDevice->waitForFences( m_vkRender_fence_syncs[ i ].get( ), true, std::numeric_limits<uint64_t>::max( ) );
            m_vkLogicalDevice->resetFences( m_vkRender_fence_syncs[ i ].get( ) );
            m_vkRender_fence_syncs[ i ]           = m_vkLogicalDevice->createFenceUnique( { vk::FenceCreateFlagBits::eSignaled } );
            m_vkSwap_chain_image_fence_syncs[ i ] = nullptr;

            (void) wait_fence_result;
            assert( wait_fence_result == vk::Result::eSuccess );
        }
    }

    /*
     *
     * sync function
     *
     * */
    inline void waitPresent( ) { m_vkPresentQueue.waitIdle( ); }
    inline void waitIdle( ) const { m_vkLogicalDevice->waitIdle( ); }

    inline vk::Instance&                 getVulkanInstance( ) { return *m_vkInstance; };
    inline vk::Device&                   getLogicalDevice( ) { return m_vkLogicalDevice.get( ); }
    inline vk::PhysicalDevice&           getPhysicalDevice( ) { return m_vkPhysicalDevice; }
    inline vk::PhysicalDeviceProperties& getPhysicalDeviceProperties( ) { return m_vkPhysicalDeviceProperties; }

    inline const std::pair<uint32_t, uint32_t>& getDesiredQueueIndices( const void* key ) const
    {
        return m_saved_queue_index.at( m_requested_queue.at( key ).first );
    }

    inline void addDesiredQueueType( const void* key, const vk::QueueFlagBits& type )
    {
        assert( !m_requested_queue.contains( key ) );
        m_requested_queue[ key ] = { 0, type };
    }

    inline auto                   GetTransferFamilyIndices( ) { return MakeMutexResources( m_vkTransfer_family_indices_mutex, m_vkTransfer_family_indices ); }
    inline auto&                  GetTransferCommandPool( ) { return m_vkTransferCommandPool; }
    inline auto&                  getMemoryAllocator( ) { return m_vkmAllocator; }
    inline auto&                  getDepthBufferImage( ) { return m_vkSwap_chain_depth_image.GetImage( ); }
    inline const auto&            getPipelineLayout( ) { return *m_vkPipeline->m_vkPipelineLayout; }
    inline auto&                  getRenderPass( ) { return *m_vkPipeline->m_vkRenderPass; }
    inline auto&                  getDescriptorPool( ) { return *m_vkPipeline->createInfo.descriptorPool; }
    inline auto&                  getDescriptorSets( ) { return m_vkPipeline->createInfo.descriptorSetsPtr; }
    inline vk::WriteDescriptorSet getWriteDescriptorSetSetup( size_t index )
    {
        assert( m_vkPipeline->createInfo.descriptorSetsPtr.size( ) > index );

        vk::WriteDescriptorSet descriptorWrite;
        descriptorWrite.setDstSet( m_vkPipeline->createInfo.descriptorSetsPtr[ index ] );
        return descriptorWrite;
    }

    inline std::size_t getSwapChainImagesCount( ) { return m_vkSwap_chain_images.size( ); }
    inline const auto& getDisplayExtent( ) { return m_vkDisplayExtent; }

    inline void invalidateSwapChain( ) { m_swap_chain_not_valid.test_and_set( ); }
    inline void setShouldCreateSwapChain( bool should )
    {
        if ( should )
        {
            m_should_create_swap_chain.test_and_set( );
            m_should_create_swap_chain.notify_one( );
        } else
        {
            m_should_create_swap_chain.clear( );
        }
    }

private:
    /**
     *
     * Window handler
     *
     * */
    GLFWwindow* m_window;

    /**
     *
     * Custom properties
     *
     * */
    std::unique_ptr<ValidationLayer> m_vkValidationLayer;
    std::unique_ptr<VulkanExtension> m_vkExtension;


    /**
     *
     * Instances
     *
     * */
    vk::UniqueInstance        m_vkInstance;
    vk::DispatchLoaderDynamic m_vkDynamicDispatch;

    /**
     *
     * Debug manager
     *
     * */
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> m_vkDebugMessenger;
    void                                                                    setupDebugManager( const vk::DebugUtilsMessengerCreateInfoEXT& );

    /**
     *
     * Physical properties
     *
     * */
    vk::Extent2D                 m_vkDisplayExtent;
    vk::PhysicalDeviceProperties m_vkPhysicalDeviceProperties;
    vk::PhysicalDevice           m_vkPhysicalDevice;
    vk::UniqueSurfaceKHR         m_vkSurface;
    vk::UniqueDevice             m_vkLogicalDevice;

    /*
     *
     * Vulkan memory allocator
     *
     * */
    VmaAllocator m_vkmAllocator;

    /**
     *
     * Queues
     *
     * */
    std::unique_ptr<QueueFamilyManager>        m_queue_family_manager;
    std::vector<std::pair<uint32_t, uint32_t>> m_saved_queue_index;
    QueueFamilyIndices                         m_vkQueue_family_indices;
    std::mutex                                 m_vkTransfer_family_indices_mutex;
    std::pair<uint32_t, uint32_t>              m_vkTransfer_family_indices;
    vk::Queue                                  m_vkGraphicQueue;
    vk::Queue                                  m_vkPresentQueue;

    /**
     *
     * Pipelines
     *
     * */
    std::unique_ptr<VulkanPipeline> m_vkPipeline;

    /**
     *
     * Swap chains
     *
     * */
    std::atomic_flag                   m_swap_chain_not_valid;
    std::atomic_flag                   m_should_create_swap_chain;
    SwapChainSupportDetails            m_vkSwap_chain_detail;
    vk::UniqueSwapchainKHR             m_vkSwap_chain;
    std::vector<vk::UniqueFramebuffer> m_vkFrameBuffers;
    std::vector<vk::Image>             m_vkSwap_chain_images;
    std::vector<vk::UniqueImageView>   m_vkSwap_chain_image_views;
    vk::Format                         m_vkSwap_chain_depth_format;

    ImageMeta m_vkSwap_chain_depth_image;

    /**
     *
     * Commands
     *
     * */
    vk::UniqueCommandPool          m_vkGraphicCommandPool;
    vk::UniqueCommandPool          m_vkTransferCommandPool;
    std::vector<vk::CommandBuffer> m_vkGraphicCommandBuffers;

    /**
     *
     * Synchronization
     *
     * */
    uint32_t                         m_sync_index = 0, m_sync_count = 1;
    std::vector<vk::UniqueSemaphore> m_vkImage_acquire_syncs;
    std::vector<vk::UniqueSemaphore> m_vkRender_syncs;
    std::vector<vk::UniqueFence>     m_vkRender_fence_syncs;
    std::vector<vk::Fence>           m_vkSwap_chain_image_fence_syncs;

    /**
     *
     * Custom detail(s)
     *
     * */
    std::unordered_map<const void*, std::pair<uint32_t, vk::QueueFlagBits>> m_requested_queue;
    std::function<void( const vk::CommandBuffer&, uint32_t index )>         m_renderer;
    std::function<void( )>                                                  m_pipeline_create_callback;
    std::array<vk::ClearValue, 2>                                           m_clearValues { vk::ClearValue { vk::ClearColorValue { std::array<float, 4> { 0.0515186f, 0.504163f, 0.656863f, 1.0f } } }, vk::ClearValue { vk::ClearDepthStencilValue { 1.f, 0 } } };
};

#include "VulkanAPI_Impl.hpp"

#endif   // MINECRAFT_VK_VULKAN_VULKANAPI_HPP
