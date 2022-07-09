//
// Created by loys on 7/7/22.
//

#ifndef MINECRAFT_VK_IMAGEMETA_HPP
#define MINECRAFT_VK_IMAGEMETA_HPP


#include <Graphic/Vulkan/VulkanAPI.hpp>
#include <Include/stb_image.h>
#include <Include/vk_mem_alloc.h>

#include <Utility/Math/Math.hpp>

#include "BufferMeta.hpp"

#include <fstream>

class ImageMeta
{

    vk::Image           image { };
    vk::UniqueImageView imageView { };
    vk::UniqueSampler   sampler { };
    VmaAllocator        allocator { };
    VmaAllocation       allocation { };

public:
    ImageMeta( )
        : allocator( VulkanAPI::GetInstance( ).getMemoryAllocator( ) )
    { }

    ~ImageMeta( )
    {
        DestroyBuffer( );
    }


    void DestroyBuffer( )
    {
        if ( image ) vmaDestroyImage( allocator, image, allocation );
        image = nullptr;
    }

    void CreateFromFile( const std::string& path );

    void CreateImageView( vk::Format format );

    void CreateSampler( bool normalizeCoordinates = false, float anisotropy = -1 );

    void Create( uint32_t imageWidth, uint32_t imageHeight, vk::Format imageFormat, vk::ImageTiling imageTiling, vk::ImageUsageFlags imageUsage, const VmaMemoryUsage& memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY, const VmaAllocationCreateFlags& memoryFlag = 0 );

    void CopyToImage( vk::DeviceSize imageSize, std::pair<int, int> imageOffset, std::pair<uint32_t, uint32_t> imageDimension, void* pixels );

    [[nodiscard]] inline auto& GetSampler( ) const
    {
        return *sampler;
    }

    [[nodiscard]] inline auto& GetImageView( ) const
    {
        return *imageView;
    }

    [[nodiscard]] inline auto& GetImage( ) const
    {
        return image;
    }
};


#endif   // MINECRAFT_VK_IMAGEMETA_HPP
