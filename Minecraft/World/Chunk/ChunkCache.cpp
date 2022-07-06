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


    //      F----H
    //  E---|G   |
    //  |  B-|--D
    //  A----C

    static constexpr auto FaceVerticesCount = 4;
    static constexpr auto FaceIndicesCount  = 6;


    static const std::array<IndexBufferType, FaceIndicesCount> blockIndices = { 0, 1, 2, 2, 3, 0 };

    static const std::array<DataType::ColoredVertex, FaceVerticesCount> FrontBlockVertices = {
        DataType::ColoredVertex {{ 1.f, 0.0f, 0.f }, { 1.0f, 0.f, 0.f }}, // B
        DataType::ColoredVertex { { 1.f, 1.f, 0.f }, { 1.0f, 0.f, 0.f }}, // F
        DataType::ColoredVertex { { 1.f, 1.f, 1.f }, { 1.0f, 0.f, 0.f }}, // H
        DataType::ColoredVertex {{ 1.f, 0.0f, 1.f }, { 1.0f, 0.f, 0.f }}, // D
    };

    static const std::array<DataType::ColoredVertex, FaceVerticesCount> BackBlockVertices = {
        DataType::ColoredVertex {{ 0.f, 0.0f, 0.f }, { 1.0f, 0.f, 0.f }}, // A
        DataType::ColoredVertex {{ 0.f, 0.0f, 1.f }, { 1.0f, 0.f, 0.f }}, // C
        DataType::ColoredVertex { { 0.f, 1.f, 1.f }, { 1.0f, 0.f, 0.f }}, // G
        DataType::ColoredVertex { { 0.f, 1.f, 0.f }, { 1.0f, 0.f, 0.f }}, // E
    };

    static const std::array<DataType::ColoredVertex, FaceVerticesCount> RightBlockVertices = {
        DataType::ColoredVertex {{ 0.f, 0.0f, 1.f }, { 0.f, 0.f, 1.f }}, // C
        DataType::ColoredVertex {{ 1.f, 0.0f, 1.f }, { 0.f, 0.f, 1.f }}, // D
        DataType::ColoredVertex { { 1.f, 1.f, 1.f }, { 0.f, 0.f, 1.f }}, // H
        DataType::ColoredVertex { { 0.f, 1.f, 1.f }, { 0.f, 0.f, 1.f }}, // G
    };

    static const std::array<DataType::ColoredVertex, FaceVerticesCount> LeftBlockVertices = {
        DataType::ColoredVertex {{ 0.f, 0.0f, 0.f }, { 0.f, 0.f, 1.f }}, // A
        DataType::ColoredVertex { { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f }}, // E
        DataType::ColoredVertex { { 1.f, 1.f, 0.f }, { 0.f, 0.f, 1.f }}, // F
        DataType::ColoredVertex {{ 1.f, 0.0f, 0.f }, { 0.f, 0.f, 1.f }}, // B
    };

    static const std::array<DataType::ColoredVertex, FaceVerticesCount> UpBlockVertices = {
        DataType::ColoredVertex {{ 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }}, // E
        DataType::ColoredVertex {{ 0.f, 1.f, 1.f }, { 0.f, 1.f, 0.f }}, // G
        DataType::ColoredVertex {{ 1.f, 1.f, 1.f }, { 0.f, 1.f, 0.f }}, // H
        DataType::ColoredVertex {{ 1.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }}, // F
    };

    static const std::array<DataType::ColoredVertex, FaceVerticesCount> DownBlockVertices = {
        DataType::ColoredVertex {{ 0.f, 0.0f, 0.f }, { 0.f, 1.f, 0.f }}, // A
        DataType::ColoredVertex {{ 1.f, 0.0f, 0.f }, { 0.f, 1.f, 0.f }}, // B
        DataType::ColoredVertex {{ 1.f, 0.0f, 1.f }, { 0.f, 1.f, 0.f }}, // D
        DataType::ColoredVertex {{ 0.f, 0.0f, 1.f }, { 0.f, 1.f, 0.f }}, // C
    };

    const auto     verticesDataSize     = ScaleToSecond<1, sizeof( DataType::ColoredVertex ) * FaceVerticesCount>( chunk.m_VisibleFacesCount );
    const auto     indicesDataSize      = ScaleToSecond<1, sizeof( IndexBufferType )>( m_IndexBufferSize = ScaleToSecond<1, FaceIndicesCount>( chunk.m_VisibleFacesCount ) );
    auto&          api                  = MainApplication::GetInstance( ).GetVulkanAPI( );
    const auto&    noiseGenerator       = MinecraftServer::GetInstance( ).GetWorld( ).GetHeightNoise( );
    const auto     suitableBufferRegion = renderBuffers.CreateBuffer( verticesDataSize, indicesDataSize );
    const uint32_t indexOffset          = ScaleToSecond<sizeof( DataType::ColoredVertex ), 1, uint32_t>( suitableBufferRegion.region.vertex.first );

    std::unique_ptr<DataType::ColoredVertex[]> chunkVertices = std::make_unique<DataType::ColoredVertex[]>( ScaleToSecond<1, FaceVerticesCount>( chunk.m_VisibleFacesCount ) );
    std::unique_ptr<IndexBufferType[]>         chunkIndices  = std::make_unique<IndexBufferType[]>( m_IndexBufferSize );

    auto AddFace = [ indexOffset, faceAdded = 0, chunkVerticesPtr = chunkVertices.get( ), chunkIndicesPtr = chunkIndices.get( ) ]( const std::array<DataType::ColoredVertex, FaceVerticesCount>& vertexArray, const glm::vec3& offset ) mutable {
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

    for ( int y = 0, blockIndex = 0; y < chunk.m_SectionHeight; ++y )
    {
        for ( int z = 0; z < SectionUnitLength; ++z )
        {
            for ( int x = 0; x < SectionUnitLength; ++x, ++blockIndex )
            {
                if ( const auto visibleFace = chunk.m_BlockFaces[ blockIndex ] )
                {
                    const auto offset = glm::vec3( chunkX + x, y, chunkZ + z );
                    if ( visibleFace & DirFrontBit )
                        AddFace( FrontBlockVertices, offset );
                    if ( visibleFace & DirBackBit )
                        AddFace( BackBlockVertices, offset );
                    if ( visibleFace & DirRightBit )
                        AddFace( RightBlockVertices, offset );
                    if ( visibleFace & DirLeftBit )
                        AddFace( LeftBlockVertices, offset );
                    if ( visibleFace & DirUpBit )
                        AddFace( UpBlockVertices, offset );
                    if ( visibleFace & DirDownBit )
                        AddFace( DownBlockVertices, offset );
                }
            }
        }
    }


    renderBuffers.CopyBuffer( suitableBufferRegion, chunkVertices.get( ), chunkIndices.get( ) );
}
