//
// Created by loys on 7/9/22.
//

#include "BlockTexture.hpp"

#include <Minecraft/World/Chunk/RenderableChunk.hpp>

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


constexpr glm::vec3 sun_dir { 1, 1, 2 };

auto
CalculateNormal( std::array<DataType::TexturedVertex, FaceVerticesCount> vertices )
{
    const glm::vec3 edge1  = vertices[ 1 ].pos - vertices[ 0 ].pos;
    const glm::vec3 edge2  = vertices[ 3 ].pos - vertices[ 0 ].pos;
    const glm::vec3 normal = glm::normalize( glm::cross( edge1, edge2 ) );

    const auto cosTheta = std::sqrt( std::clamp( glm::dot( glm::normalize( sun_dir ), normal ), 0.1f, 1.f ) );
    for ( auto& vertex : vertices )
        vertex.textureCoor_ColorIntensity.z = cosTheta * vertex.textureCoor_ColorIntensity.z;
    return vertices;
}

#define TEXTURED_VERTEX( A, B, C, D, E,                 \
                         F, I, J, K, L,                 \
                         M, N, O, P, Q,                 \
                         R, S, T, U, V )                \
    DataType::TexturedVertex { { A, B, C }, { D, E } }, \
    DataType::TexturedVertex { { F, I, J }, { K, L } }, \
    DataType::TexturedVertex { { M, N, O }, { P, Q } }, \
    DataType::TexturedVertex { { R, S, T }, { U, V } }

inline const BlockTexture::TextureLocation defaultBlockVertices {
    CalculateNormal( std::array<DataType::TexturedVertex, FaceVerticesCount> {
        TEXTURED_VERTEX( 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,      // D
                         1.0f, 0.0f, 0.0f, 1.0f, 0.0f,      // B
                         1.0f, 1.0f, 0.0f, 1.0f, 1.0f,      // F
                         1.0f, 1.0f, 1.0f, 0.0f, 1.0f ) }   // H
                     ),
    CalculateNormal( std::array<DataType::TexturedVertex, FaceVerticesCount> {
        TEXTURED_VERTEX( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,      // A
                         0.0f, 0.0f, 1.0f, 1.0f, 0.0f,      // C
                         0.0f, 1.0f, 1.0f, 1.0f, 1.0f,      // G
                         0.0f, 1.0f, 0.0f, 0.0f, 1.0f ) }   // E
                     ),
    CalculateNormal( std::array<DataType::TexturedVertex, FaceVerticesCount> {
        TEXTURED_VERTEX( 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,      // C
                         1.0f, 0.0f, 1.0f, 1.0f, 0.0f,      // D
                         1.0f, 1.0f, 1.0f, 1.0f, 1.0f,      // H
                         0.0f, 1.0f, 1.0f, 0.0f, 1.0f ) }   // G
                     ),
    CalculateNormal( std::array<DataType::TexturedVertex, FaceVerticesCount> {
        TEXTURED_VERTEX( 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,      // B
                         0.0f, 0.0f, 0.0f, 1.0f, 0.0f,      // A
                         0.0f, 1.0f, 0.0f, 1.0f, 1.0f,      // E
                         1.0f, 1.0f, 0.0f, 0.0f, 1.0f ) }   // F
                     ),
    CalculateNormal( std::array<DataType::TexturedVertex, FaceVerticesCount> {
        TEXTURED_VERTEX( 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,      // E
                         0.0f, 1.0f, 1.0f, 1.0f, 0.0f,      // G
                         1.0f, 1.0f, 1.0f, 1.0f, 1.0f,      // H
                         1.0f, 1.0f, 0.0f, 0.0f, 1.0f ) }   // F
                     ),
    CalculateNormal( std::array<DataType::TexturedVertex, FaceVerticesCount> {
        TEXTURED_VERTEX( 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,      // B
                         1.0f, 0.0f, 1.0f, 1.0f, 0.0f,      // D
                         0.0f, 0.0f, 1.0f, 1.0f, 1.0f,      // C
                         0.0f, 0.0f, 0.0f, 0.0f, 1.0f ) }   // A
                     ) };

#undef TEXTURED_VERTEX

}   // namespace

BlockTexture::BlockTexture( const std::string& folder )
{
    std::filesystem::path textureJson = std::filesystem::path( folder + "/texture.json" ).make_preferred( ).string( );
    std::ifstream         file( textureJson );
    if ( !file ) throw std::runtime_error( "missing file " + textureJson.string( ) );

    nlohmann::json textureSpec = nlohmann::json::parse( file,
                                                        /* callback */ nullptr,
                                                        /* allow exceptions */ true,
                                                        /* ignore_comments */ true );

    Logger::getInstance( ).LogLine( "Using texture", textureSpec[ "texture_name" ] );

    m_UniqueTexture.clear( );
    const auto& blockTexturesJson = textureSpec[ "blocks" ];
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
            const auto& texturePath = std::filesystem::path( folder + '/' + textureName ).make_preferred( ).string( );
            if ( m_UniqueTexture.contains( texturePath ) ) continue;
            m_UniqueTexture[ texturePath ] = m_UniqueTexture.size( );
        }
    }

    const auto     totalTexture         = m_UniqueTexture.size( );
    const uint32_t textureAtlasesWidth  = std::floor( std::sqrt( totalTexture ) );
    const uint32_t textureAtlasesHeight = std::ceil( (float) totalTexture / textureAtlasesWidth );

    const auto textureResolution = textureSpec[ "resolution" ].get<uint32_t>( );

    textureImage.SetAllocator( );
    textureImage.Create( textureAtlasesWidth * textureResolution, textureAtlasesHeight * textureResolution, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, VMA_MEMORY_USAGE_GPU_ONLY, VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT );

    uint32_t index = 0;
    for ( auto& texturePath : m_UniqueTexture )
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

        for ( int j = 0; j < DirSize; ++j )
        {
            const auto& texturePath = std::filesystem::path( folder + '/' + textureList[ j ].get<std::string>( ) ).make_preferred( ).string( );

            index = m_UniqueTexture.at( texturePath );

            uint32_t x = ( index % textureAtlasesWidth ), y = ( index / textureAtlasesWidth );

            glm::vec2 offset { textureResolution * x, textureResolution * y };

            static_assert( FaceVerticesCount == 4 );
            std::array<DataType::TexturedVertex, FaceVerticesCount> textureFace = defaultBlockVertices[ j ];

            textureFace[ 0 ].SetTextureCoor( offset );
            textureFace[ 0 ].SetAccumulatedTextureCoor( glm::vec2 { 0 } );

            textureFace[ 1 ].SetTextureCoor( offset );
            textureFace[ 1 ].SetAccumulatedTextureCoor( glm::vec2 { textureResolution, 0 } );

            textureFace[ 2 ].SetTextureCoor( offset );
            textureFace[ 2 ].SetAccumulatedTextureCoor( glm::vec2 { textureResolution, textureResolution } );

            textureFace[ 3 ].SetTextureCoor( offset );
            textureFace[ 3 ].SetAccumulatedTextureCoor( glm::vec2 { 0, textureResolution } );

            m_BlockTextureIndices[ i ][ j ] = m_TextureList.size( );
            m_TextureList.push_back( textureFace );

            //            m_BlockTextures[ i ][ j ][ 0 ].textureCoor = {0, 0};
            //            m_BlockTextures[ i ][ j ][ 1 ].textureCoor = {1, 0};
            //            m_BlockTextures[ i ][ j ][ 2 ].textureCoor = {1, 1};
            //            m_BlockTextures[ i ][ j ][ 3 ].textureCoor = {0, 1};
        }
    }

    file.close( );

    if ( m_TextureList.size( ) >= FaceVertexMetaData::GetMaxTextureIDSupported( ) )
    {
        throw std::runtime_error( "m_TextureList.size() >= FaceVertexMetaData::GetMaxTextureIDSupported()" );
    }
}