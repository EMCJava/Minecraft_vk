//
// Created by loys on 1/24/2022.
//

#ifndef MINECRAFT_VK_VULKAN_VULKANAPI_HPP
#define MINECRAFT_VK_VULKAN_VULKANAPI_HPP

#include <Include/GraphicAPI.hpp>
#include <Include/vk_mem_alloc.h>

#include <Graphic/Vulkan/Pipeline/VulkanPipeline.hpp>
#include <Utility/Vulkan/ValidationLayer.hpp>
#include <Utility/Vulkan/VulkanExtension.hpp>

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
    glm::vec2  textureCoor;


    TexturedVertex( glm::vec3 p = glm::vec3( 0 ), glm::vec2 c = glm::vec2( 0 ) )
        : pos( p )
        , textureCoor( c )
    { }

    TexturedVertex& operator=( const TexturedVertex& other )
    {
        pos         = other.pos;
        textureCoor = other.textureCoor;

        return *this;
    }

    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions( )
    {
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions( 2 );

        attributeDescriptions[ 0 ].setBinding( 0 );
        attributeDescriptions[ 0 ].setLocation( 0 );
        attributeDescriptions[ 0 ].setFormat( vk::Format::eR32G32B32Sint );
        attributeDescriptions[ 0 ].setOffset( offsetof( TexturedVertex, pos ) );

        attributeDescriptions[ 1 ].setBinding( 0 );
        attributeDescriptions[ 1 ].setLocation( 1 );
        attributeDescriptions[ 1 ].setFormat( vk::Format::eR32G32Sfloat );
        attributeDescriptions[ 1 ].setOffset( offsetof( TexturedVertex, textureCoor ) );

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
public:
    struct VKBufferMeta {

        vk::UniqueBuffer                   buffer;
        vk::MemoryRequirements             memRequirements;
        vk::PhysicalDeviceMemoryProperties memProperties;
        vk::UniqueDeviceMemory             deviceMemory;

        inline auto& GetBuffer( ) const
        {
            return *buffer;
        }

        static uint32_t FindMemoryType( const vk::PhysicalDeviceMemoryProperties& memProperties, uint32_t typeFilter, vk::MemoryPropertyFlags properties )
        {
            for ( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ )
                if ( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[ i ].propertyFlags & properties ) == properties ) return i;
            throw std::runtime_error( "failed to find suitable memory type!" );
        }

        void Create( const vk::DeviceSize size, const vk::BufferUsageFlags usage, const VulkanAPI& api, const vk::MemoryPropertyFlags memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     const vk::SharingMode sharingMode = vk::SharingMode::eExclusive )
        {
            buffer          = api.m_vkLogicalDevice->createBufferUnique( { { }, size, usage, sharingMode } );
            memRequirements = api.m_vkLogicalDevice->getBufferMemoryRequirements( buffer.get( ) );
            memProperties   = api.m_vkPhysicalDevice.getMemoryProperties( );

            deviceMemory = api.m_vkLogicalDevice->allocateMemoryUnique( { memRequirements.size, FindMemoryType( memProperties, memRequirements.memoryTypeBits, memoryProperties ) } );
            api.m_vkLogicalDevice->bindBufferMemory( buffer.get( ), deviceMemory.get( ), 0 );
        }

        void writeBuffer( const void* writingData, size_t dataSize, const VulkanAPI& api )
        {
            writeBufferOffseted( writingData, dataSize, 0, api );
        }

        void writeBufferOffseted( const void* writingData, size_t dataSize, size_t offset, const VulkanAPI& api )
        {
            auto* bindingPoint = api.m_vkLogicalDevice->mapMemory( deviceMemory.get( ), offset, dataSize );
            memcpy( bindingPoint, writingData, dataSize );
            api.m_vkLogicalDevice->unmapMemory( deviceMemory.get( ) );
        }

        void CopyFromBuffer( const VKBufferMeta& bufferData, const vk::ArrayProxy<const vk::BufferCopy>& dataRegion, const VulkanAPI& api )
        {
            static std::mutex           copy_buffer_lock;
            std::lock_guard<std::mutex> guard( copy_buffer_lock );

            vk::CommandBufferAllocateInfo allocInfo { };
            allocInfo.setCommandPool( *api.m_vkTransferCommandPool );
            allocInfo.setLevel( vk::CommandBufferLevel::ePrimary );
            allocInfo.setCommandBufferCount( 1 );

            auto  commandBuffers = api.m_vkLogicalDevice->allocateCommandBuffersUnique( allocInfo );
            auto& commandBuffer  = commandBuffers.begin( )->get( );

            commandBuffer.begin( { vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );
            commandBuffer.copyBuffer( *bufferData.buffer, *buffer, dataRegion );
            commandBuffer.end( );

            const auto& transferFamilyIndices = api.m_vkTransfer_family_indices;
            auto        transferQueue         = api.m_vkLogicalDevice->getQueue( transferFamilyIndices.first, transferFamilyIndices.second );

            vk::SubmitInfo submitInfo;
            submitInfo.setCommandBuffers( commandBuffer );
            transferQueue.submit( submitInfo, nullptr );
            transferQueue.waitIdle( );
        }
    };

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

    vk::UniqueImage createImage( uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueDeviceMemory& imageMemory );

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
        for ( int i = 0;i < m_vkRender_fence_syncs.size(); ++i )
        {
            std::cout << "Waiting " << i << std::endl;
            auto wait_fence_result = m_vkLogicalDevice->waitForFences( m_vkRender_fence_syncs[i].get( ), true, std::numeric_limits<uint64_t>::max( ) );
            m_vkLogicalDevice->resetFences( m_vkRender_fence_syncs[i].get( ) );
            m_vkRender_fence_syncs[i] = m_vkLogicalDevice->createFenceUnique( { vk::FenceCreateFlagBits::eSignaled } );
            m_vkSwap_chain_image_fence_syncs[i] = nullptr;
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

    auto&                         GetTransferFamilyIndices( ) { return m_vkTransfer_family_indices; }
    auto&                         GetTransferCommandPool( ) { return m_vkTransferCommandPool; }
    auto&                         getMemoryAllocator( ) { return m_vkmAllocator; }
    inline auto&                  getDepthBufferImage( ) { return m_vkSwap_chain_depth_image.get( ); }
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
    vk::UniqueSurfaceKHR         m_vkSurface;
    vk::UniqueDevice             m_vkLogicalDevice;
    vk::PhysicalDevice           m_vkPhysicalDevice;
    vk::PhysicalDeviceProperties m_vkPhysicalDeviceProperties;

    /**
     *
     * Queues
     *
     * */
    std::unique_ptr<QueueFamilyManager>        m_queue_family_manager;
    std::vector<std::pair<uint32_t, uint32_t>> m_saved_queue_index;
    QueueFamilyIndices                         m_vkQueue_family_indices;
    std::pair<uint32_t, uint32_t>              m_vkTransfer_family_indices;
    vk::Queue                                  m_vkGraphicQueue;
    vk::Queue                                  m_vkPresentQueue;

    /**
     *
     * Swap chains
     *
     * */
    std::atomic_flag                 m_swap_chain_not_valid;
    std::atomic_flag                 m_should_create_swap_chain;
    SwapChainSupportDetails          m_vkSwap_chain_detail;
    vk::UniqueSwapchainKHR           m_vkSwap_chain;
    std::vector<vk::Image>           m_vkSwap_chain_images;
    std::vector<vk::UniqueImageView> m_vkSwap_chain_image_views;
    vk::Format                       m_vkSwap_chain_depth_format;
    vk::UniqueImage                  m_vkSwap_chain_depth_image;
    vk::UniqueDeviceMemory           m_vkSwap_chain_depth_memory;
    vk::UniqueImageView              m_vkSwap_chain_depth_view;

    /**
     *
     * Pipelines
     *
     * */
    std::unique_ptr<VulkanPipeline>    m_vkPipeline;
    std::vector<vk::UniqueFramebuffer> m_vkFrameBuffers;

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

    /*
     *
     * Vulkan memory allocator
     *
     * */
    VmaAllocator m_vkmAllocator;

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
