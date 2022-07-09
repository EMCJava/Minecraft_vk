//
// Created by loys on 5/24/22.
//

#include <Minecraft/Application/MainApplication.hpp>
#include <Minecraft/util/MinecraftConstants.hpp>

#include <Graphic/Vulkan/VulkanAPI.hpp>

#include <Utility/Logger.hpp>

#include "ChunkCache.hpp"

void
ChunkCache::ResetLoad( )
{
    chunk.RegenerateChunk( );
}

void
ChunkCache::ResetModel( ChunkSolidBuffer& renderBuffers )
{
    if ( chunk.m_VisibleFacesCount == 0 ) return;

    // Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Generating chunk:", chunk.GetCoordinate( ) );

    auto [ chunkX, chunkZ, chunkY ] = chunk.GetCoordinate( );

    chunkX <<= SectionUnitLengthBinaryOffset;
    chunkZ <<= SectionUnitLengthBinaryOffset;

    static const std::array<IndexBufferType, FaceIndicesCount> blockIndices = { 0, 1, 2, 2, 3, 0 };

    const auto     verticesDataSize     = ScaleToSecond<1, sizeof( DataType::TexturedVertex ) * FaceVerticesCount>( chunk.m_VisibleFacesCount );
    const auto     indicesDataSize      = ScaleToSecond<1, sizeof( IndexBufferType )>( m_IndexBufferSize = ScaleToSecond<1, FaceIndicesCount>( chunk.m_VisibleFacesCount ) );
    auto&          api                  = MainApplication::GetInstance( ).GetVulkanAPI( );
    const auto&    noiseGenerator       = MinecraftServer::GetInstance( ).GetWorld( ).GetTerrainNoise( );
    const auto     suitableBufferRegion = renderBuffers.CreateBuffer( verticesDataSize, indicesDataSize );
    const uint32_t indexOffset          = ScaleToSecond<sizeof( DataType::TexturedVertex ), 1, uint32_t>( suitableBufferRegion.region.vertex.first );

    std::unique_ptr<DataType::TexturedVertex[]> chunkVertices = std::make_unique<DataType::TexturedVertex[]>( ScaleToSecond<1, FaceVerticesCount>( chunk.m_VisibleFacesCount ) );
    std::unique_ptr<IndexBufferType[]>          chunkIndices  = std::make_unique<IndexBufferType[]>( m_IndexBufferSize );

    auto AddFace = [ indexOffset, faceAdded = 0, chunkVerticesPtr = chunkVertices.get( ), chunkIndicesPtr = chunkIndices.get( ) ]( const std::array<DataType::TexturedVertex, FaceVerticesCount>& vertexArray, const glm::vec3& offset ) mutable {
        for ( int k = 0; k < FaceVerticesCount; ++k, ++chunkVerticesPtr )
        {
            *chunkVerticesPtr = vertexArray[ k ];
            chunkVerticesPtr->pos += offset;
        }

        for ( int k = 0; k < FaceIndicesCount; ++k, ++chunkIndicesPtr )
        {
            *chunkIndicesPtr = blockIndices[ k ] + faceAdded * FaceVerticesCount + indexOffset;
        }

        ++faceAdded;
    };

    const auto& blockTextures = Minecraft::GetInstance( ).GetBlockTextures( );

    for ( int y = 0, blockIndex = 0; y < ChunkMaxHeight; ++y )
    {
        for ( int z = 0; z < SectionUnitLength; ++z )
        {
            for ( int x = 0; x < SectionUnitLength; ++x, ++blockIndex )
            {
                if ( const auto visibleFace = chunk.m_BlockFaces[ blockIndex ] )
                {
                    auto       textures = blockTextures.GetTextureLocation( chunk.m_Blocks[ blockIndex ] );
                    const auto offset   = glm::vec3( chunkX + x, y, chunkZ + z );
                    if ( visibleFace & DirFrontBit )
                        AddFace( textures[ DirFront ], offset );
                    if ( visibleFace & DirBackBit )
                        AddFace( textures[ DirBack ], offset );
                    if ( visibleFace & DirRightBit )
                        AddFace( textures[ DirRight ], offset );
                    if ( visibleFace & DirLeftBit )
                        AddFace( textures[ DirLeft ], offset );
                    if ( visibleFace & DirUpBit )
                        AddFace( textures[ DirUp ], offset );
                    if ( visibleFace & DirDownBit )
                        AddFace( textures[ DirDown ], offset );
                }
            }
        }
    }

    renderBuffers.CopyBuffer( suitableBufferRegion, chunkVertices.get( ), chunkIndices.get( ) );
}
