//
// Created by loys on 7/9/22.
//

#ifndef MINECRAFT_VK_BLOCKTEXTURE_HPP
#define MINECRAFT_VK_BLOCKTEXTURE_HPP

#include "Block.hpp"

#include <Graphic/Vulkan/VulkanAPI.hpp>

#include <Minecraft/util/MinecraftConstants.hpp>

#include <Graphic/Vulkan/ImageMeta.hpp>

#include <string>
#include <utility>

class BlockTexture
{
public:
    using TextureLocation = std::array<std::array<DataType::TexturedVertex, FaceVerticesCount>, DirSize>;

private:
    ImageMeta textureImage;

    std::array<TextureLocation, BlockIDSize> m_BlockTextures;

public:
    BlockTexture( const std::string& folder );

    inline const auto& GetTextureLocation( BlockID id ) const { return m_BlockTextures.at( id ); }

    const auto& GetTexture( ) const { return textureImage; }
};


#endif   // MINECRAFT_VK_BLOCKTEXTURE_HPP
