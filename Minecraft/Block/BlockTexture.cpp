//
// Created by loys on 7/9/22.
//

#include "BlockTexture.hpp"

#include <Include/nlohmann/json.hpp>
#include <Utility/Logger.hpp>

#include <Include/stb_image.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>

namespace
{

//     F----H
//  E--|-G  |
//  |  B-|--D
//  A----C

inline const BlockTexture::TextureLocation defaultBlockVertices {
    std::array<DataType::TexturedVertex, FaceVerticesCount> {
                                                             DataType::TexturedVertex { { 1.f, 0.0f, 1.f }, { 0.0f, 0.f } }, // D
 DataType::TexturedVertex { { 1.f, 0.0f, 0.f }, { 1.0f, 0.f } }, // B
 DataType::TexturedVertex { { 1.f, 1.f, 0.f }, { 1.0f, 1.f } }, // F
 DataType::TexturedVertex { { 1.f, 1.f, 1.f }, { 0.0f, 1.f } },  // H
    },
    std::array<DataType::TexturedVertex, FaceVerticesCount> {
                                                             DataType::TexturedVertex { { 0.f, 0.0f, 0.f }, { 0.0f, 0.f } }, // A
 DataType::TexturedVertex { { 0.f, 0.0f, 1.f }, { 1.0f, 0.f } }, // C
 DataType::TexturedVertex { { 0.f, 1.f, 1.f }, { 1.0f, 1.f } },      // G
      DataType::TexturedVertex { { 0.f, 1.f, 0.f }, { 0.0f, 1.f } },  // E
    },
    std::array<DataType::TexturedVertex, FaceVerticesCount> {
                                                             DataType::TexturedVertex { { 0.f, 0.0f, 1.f }, { 0.f, 0.f } },  // C
  DataType::TexturedVertex { { 1.f, 0.0f, 1.f }, { 1.f, 0.f } },  // D
  DataType::TexturedVertex { { 1.f, 1.f, 1.f }, { 1.f, 1.f } },      // H
      DataType::TexturedVertex { { 0.f, 1.f, 1.f }, { 0.f, 1.f } },  // G
    },
    std::array<DataType::TexturedVertex, FaceVerticesCount> {
                                                             DataType::TexturedVertex { { 1.f, 0.0f, 0.f }, { 0.f, 0.f } },  // B
  DataType::TexturedVertex { { 0.f, 0.0f, 0.f }, { 1.f, 0.f } },  // A
  DataType::TexturedVertex { { 0.f, 1.f, 0.f }, { 1.f, 1.f } },      // E
      DataType::TexturedVertex { { 1.f, 1.f, 0.f }, { 0.f, 1.f } },  // F
    },
    std::array<DataType::TexturedVertex, FaceVerticesCount> {
                                                             DataType::TexturedVertex { { 0.f, 1.f, 0.f }, { 0.f, 0.f } },   // E
   DataType::TexturedVertex { { 0.f, 1.f, 1.f }, { 1.f, 0.f } },  // G
  DataType::TexturedVertex { { 1.f, 1.f, 1.f }, { 1.f, 1.f } },      // H
      DataType::TexturedVertex { { 1.f, 1.f, 0.f }, { 0.f, 1.f } }, // F
    },
    std::array<DataType::TexturedVertex, FaceVerticesCount> {

                                                             DataType::TexturedVertex { { 1.f, 0.0f, 0.f }, { 1.f, 0.f } },  // B
  DataType::TexturedVertex { { 1.f, 0.0f, 1.f }, { 0.f, 0.f } }, // D
 DataType::TexturedVertex { { 0.f, 0.0f, 1.f }, { 0.f, 1.f } },      // C
      DataType::TexturedVertex { { 0.f, 0.0f, 0.f }, { 1.f, 1.f } }, // A
    }
};

}   // namespace

BlockTexture::BlockTexture( const std::string& folder )
{
    std::filesystem::path textureJson = folder + std::filesystem::path::preferred_separator + "texture.json";
    std::ifstream         file( textureJson );
    if ( !file ) throw std::runtime_error( "missing file " + textureJson.string( ) );

    nlohmann::json textureSpec = nlohmann::json::parse( file,
                                                        /* callback */ nullptr,
                                                        /* allow exceptions */ true,
                                                        /* ignore_comments */ true );

    Logger::getInstance( ).LogLine( "Using texture", textureSpec[ "texture_name" ] );

    std::map<std::string, uint32_t> uniqueTexture;
    const auto&                     blockTexturesJson = textureSpec[ "blocks" ];
    for ( int i = 0; i < BlockIDSize; ++i )
    {
        const auto blockName = toString( static_cast<BlockID>( i ) );
        if ( !blockTexturesJson.contains( blockName ) )
        {
            Logger::getInstance( ).LogLine( "Skipping", blockName, "texture" );
            continue;
        }

        const auto& textureList = blockTexturesJson[ blockName ];
        for ( int j = 0; j < DirSize; ++j )
        {
            const auto& textureName = textureList[ j ].get<std::string>( );
            const auto& texturePath = folder + std::filesystem::path::preferred_separator + textureName;
            if ( uniqueTexture.contains( texturePath ) ) continue;
            uniqueTexture[ texturePath ] = uniqueTexture.size( );
        }
    }

    const auto     totalTexture         = uniqueTexture.size( );
    const uint32_t textureAtlasesWidth  = std::floor( std::sqrt( totalTexture ) );
    const uint32_t textureAtlasesHeight = std::ceil( (float) totalTexture / textureAtlasesWidth );

    const auto textureResolution = textureSpec[ "resolution" ].get<uint32_t>( );

    textureImage.Create( textureAtlasesWidth * textureResolution, textureAtlasesHeight * textureResolution, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, VMA_MEMORY_USAGE_GPU_ONLY, VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT );

    uint32_t index = 0;
    for ( auto& texturePath : uniqueTexture )
    {
        texturePath.second = index;

        if ( !std::filesystem::exists( texturePath.first ) ) throw std::runtime_error( "missing file " + texturePath.first );

        static constexpr int ColorTy = STBI_rgb_alpha;
        int                  texWidth, texHeight, texChannels;
        stbi_set_flip_vertically_on_load( true );
        stbi_uc*       pixels    = stbi_load( texturePath.first.c_str( ), &texWidth, &texHeight, &texChannels, ColorTy );
        vk::DeviceSize imageSize = ScaleToSecond<1, ColorTy>( texWidth * texHeight );

        std::pair<int, int> offset { textureResolution * ( index % textureAtlasesWidth ), textureResolution * ( index / textureAtlasesWidth ) };
        textureImage.CopyToImage( imageSize, offset, { textureResolution, textureResolution }, pixels );
        stbi_image_free( pixels );

        ++index;
    }

    textureImage.CreateImageView( vk::Format::eR8G8B8A8Srgb );
    textureImage.CreateSampler( false, -1 );

    for ( int i = 0; i < BlockIDSize; ++i )
    {
        const auto blockName = toString( static_cast<BlockID>( i ) );
        if ( !blockTexturesJson.contains( blockName ) ) continue;

        const auto& textureList = blockTexturesJson[ blockName ];

        m_BlockTextures[ i ] = defaultBlockVertices;

        for ( int j = 0; j < DirSize; ++j )
        {
            const auto& texturePath = folder + std::filesystem::path::preferred_separator + textureList[ j ].get<std::string>( );

            index      = uniqueTexture.at( texturePath );
            uint32_t x = ( index % textureAtlasesWidth ), y = ( index / textureAtlasesWidth );

            glm::vec2 offset { textureResolution * x, textureResolution * y };

            static_assert( FaceVerticesCount == 4 );
            m_BlockTextures[ i ][ j ][ 0 ].textureCoor = offset;
            m_BlockTextures[ i ][ j ][ 1 ].textureCoor = offset + glm::vec2 { textureResolution, 0 };
            m_BlockTextures[ i ][ j ][ 2 ].textureCoor = offset + glm::vec2 { textureResolution, textureResolution };
            m_BlockTextures[ i ][ j ][ 3 ].textureCoor = offset + glm::vec2 { 0, textureResolution };

            //            m_BlockTextures[ i ][ j ][ 0 ].textureCoor = {0, 0};
            //            m_BlockTextures[ i ][ j ][ 1 ].textureCoor = {1, 0};
            //            m_BlockTextures[ i ][ j ][ 2 ].textureCoor = {1, 1};
            //            m_BlockTextures[ i ][ j ][ 3 ].textureCoor = {0, 1};
        }
    }

    file.close( );
}