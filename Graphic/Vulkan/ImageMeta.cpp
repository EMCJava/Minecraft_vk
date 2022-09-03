//
// Created by loys on 7/7/22.
//

#include "ImageMeta.hpp"

#include <Graphic/Vulkan/VulkanAPI.hpp>
#include <Include/stb_image_Impl.cpp>

void
ImageMeta::CreateFromFile( const std::string& path )
{
    static constexpr int ColorTy = STBI_rgb_alpha;
    int                  texWidth, texHeight, texChannels;
    stbi_set_flip_vertically_on_load( true );
    stbi_uc*       pixels    = stbi_load( path.c_str( ), &texWidth, &texHeight, &texChannels, ColorTy );
    vk::DeviceSize imageSize = ScaleToSecond<1, ColorTy>( texWidth * texHeight );

    if ( !pixels ) throw std::runtime_error( "failed to load texture image!" );

    static_assert( ColorTy == STBI_rgb_alpha );
    Create( texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, VMA_MEMORY_USAGE_GPU_ONLY, VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT );

    CopyToImage( imageSize, { 0, 0 }, { texWidth, texHeight }, pixels );
    stbi_image_free( pixels );

    CreateImageView( vk::Format::eR8G8B8A8Srgb );
    CreateSampler( false, -1 );
}

void
ImageMeta::CreateImageView( vk::Format format, vk::ImageAspectFlags imageAspectFlags )
{

    vk::ImageViewCreateInfo imageViewCreateInfo;

    imageViewCreateInfo.setImage( image );
    imageViewCreateInfo.setViewType( vk::ImageViewType::e2D );
    imageViewCreateInfo.setFormat( format );
    imageViewCreateInfo.setSubresourceRange( { imageAspectFlags, 0, 1, 0, 1 } );

    imageView = VulkanAPI::GetInstance( ).getLogicalDevice( ).createImageViewUnique( imageViewCreateInfo );
}

void
ImageMeta::CreateSampler( bool normalizeCoordinates, float anisotropy )
{

    vk::SamplerCreateInfo createInfo;

    createInfo.setMagFilter( vk::Filter::eNearest );   // texture too small
    createInfo.setMinFilter( vk::Filter::eNearest );   // texture too big

    createInfo.setAddressModeU( vk::SamplerAddressMode::eClampToBorder );
    createInfo.setAddressModeV( vk::SamplerAddressMode::eClampToBorder );
    createInfo.setAddressModeW( vk::SamplerAddressMode::eClampToBorder );

    createInfo.setAnisotropyEnable( anisotropy > 0 );
    createInfo.setMaxAnisotropy( anisotropy > 0 ? anisotropy : 0 );

    createInfo.setBorderColor( vk::BorderColor::eIntOpaqueWhite );

    createInfo.setUnnormalizedCoordinates( !normalizeCoordinates );

    createInfo.setCompareEnable( false );
    createInfo.setCompareOp( vk::CompareOp::eAlways );

    createInfo.setMipmapMode( anisotropy > 0 ? vk::SamplerMipmapMode::eLinear : vk::SamplerMipmapMode::eNearest );
    createInfo.setMipLodBias( 0.f );
    createInfo.setMinLod( 0.f );
    createInfo.setMaxLod( 0.f );

    sampler = VulkanAPI::GetInstance( ).getLogicalDevice( ).createSamplerUnique( createInfo );
}

void
ImageMeta::Create( uint32_t imageWidth, uint32_t imageHeight, vk::Format imageFormat, vk::ImageTiling imageTiling, vk::ImageUsageFlags imageUsage, const VmaMemoryUsage& memoryUsage, const VmaAllocationCreateFlags& memoryFlag )
{
    DestroyBuffer( );

    vk::ImageCreateInfo creatInfo;
    creatInfo
        .setImageType( vk::ImageType::e2D )
        .setExtent( { imageWidth, imageHeight, 1 } )
        .setMipLevels( 1 )
        .setArrayLayers( 1 )
        .setFormat( imageFormat )
        .setTiling( imageTiling )
        .setInitialLayout( vk::ImageLayout::eUndefined )
        .setUsage( imageUsage )
        .setSamples( vk::SampleCountFlagBits::e1 )
        .setSharingMode( vk::SharingMode::eExclusive );

    VmaAllocationCreateInfo allocInfo = { };
    allocInfo.flags                   = memoryFlag;
    allocInfo.usage                   = memoryUsage;

    vmaCreateImage( allocator, reinterpret_cast<VkImageCreateInfo*>( &creatInfo ), &allocInfo, reinterpret_cast<VkImage*>( &image ), &allocation, nullptr );
}

void
ImageMeta::CopyToImage( vk::DeviceSize imageSize, std::pair<int, int> imageOffset, std::pair<uint32_t, uint32_t> imageDimension, void* pixels )
{
    using Usage = vk::BufferUsageFlagBits;

    BufferMeta stagingBuffer;
    stagingBuffer.SetAllocator( );
    stagingBuffer.Create( imageSize, Usage::eTransferSrc );
    stagingBuffer.writeBuffer( pixels, imageSize );

    static std::mutex           copy_buffer_lock;
    std::lock_guard<std::mutex> guard( copy_buffer_lock );

    // this will lock the transfer queue
    const auto& transferFamilyIndicesResources = VulkanAPI::GetInstance( ).GetTransferFamilyIndices( );
    auto        transferQueue                  = VulkanAPI::GetInstance( ).getLogicalDevice( ).getQueue( transferFamilyIndicesResources.resources.first, transferFamilyIndicesResources.resources.second );

    vk::CommandBufferAllocateInfo allocInfo { };
    allocInfo.setCommandPool( *VulkanAPI::GetInstance( ).GetTransferCommandPool( ) );
    allocInfo.setLevel( vk::CommandBufferLevel::ePrimary );
    allocInfo.setCommandBufferCount( 1 );

    auto  commandBuffers = VulkanAPI::GetInstance( ).getLogicalDevice( ).allocateCommandBuffersUnique( allocInfo );
    auto& commandBuffer  = commandBuffers.begin( )->get( );

    commandBuffer.begin( { vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );

    vk::ImageMemoryBarrier imageTransCopyBarrier;
    imageTransCopyBarrier.setImage( image )
        .setOldLayout( vk::ImageLayout::eUndefined )
        .setNewLayout( vk::ImageLayout::eTransferDstOptimal )
        .setSrcQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setDstQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setSubresourceRange( { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } )
        .setSrcAccessMask( { } )
        .setDstAccessMask( vk::AccessFlagBits::eTransferWrite );


    commandBuffer.pipelineBarrier( vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, { },
                                   { },
                                   { },
                                   imageTransCopyBarrier );

    vk::BufferImageCopy imageRegion;
    imageRegion.setBufferOffset( 0 )
        .setBufferRowLength( 0 )
        .setBufferImageHeight( 0 );

    imageRegion.setImageSubresource( { vk::ImageAspectFlagBits::eColor, 0, 0, 1 } );
    imageRegion.setImageOffset( { imageOffset.first, imageOffset.second, 0 } );
    imageRegion.setImageExtent( { imageDimension.first, imageDimension.second, 1 } );

    commandBuffer.copyBufferToImage( stagingBuffer.GetBuffer( ), image, vk::ImageLayout::eTransferDstOptimal, imageRegion );

    vk::ImageMemoryBarrier imageTransReadBarrier;
    imageTransReadBarrier.setImage( image )
        .setOldLayout( vk::ImageLayout::eUndefined )
        .setNewLayout( vk::ImageLayout::eShaderReadOnlyOptimal )
        .setSrcQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setDstQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setSubresourceRange( { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } )
        .setSrcAccessMask( vk::AccessFlagBits::eTransferWrite )
        .setDstAccessMask( vk::AccessFlagBits::eShaderRead );


    commandBuffer.pipelineBarrier( vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, { },
                                   { },
                                   { },
                                   imageTransReadBarrier );

    commandBuffer.end( );

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers( commandBuffer );
    transferQueue.submit( submitInfo, nullptr );
    transferQueue.waitIdle( );
}

void
ImageMeta::SetAllocator( VmaAllocator newAllocator )
{
    allocator = newAllocator ? newAllocator : VulkanAPI::GetInstance( ).getMemoryAllocator( );
}
