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
    using TextureIndices = std::array<uint32_t, DirSize>;

private:
    ImageMeta                       textureImage;
    std::map<std::string, uint32_t> m_UniqueTexture;

    std::vector<std::array<DataType::TexturedVertex, FaceVerticesCount>> m_TextureList;

    std::array<TextureIndices, BlockIDSize> m_BlockTextureIndices { };

public:
    BlockTexture( const std::string& folder );

    inline const auto& GetTextureLocationByIndex( uint32_t id ) const { return m_TextureList.at( id ); }
    inline const auto& GetTextureIndices( BlockID id ) const { return m_BlockTextureIndices.at( id ); }

    inline auto        GetTotalUniqueTexture( ) const { return m_UniqueTexture.size( ); }
    inline const auto& GetTexture( ) const { return textureImage; }
};


#endif   // MINECRAFT_VK_BLOCKTEXTURE_HPP
