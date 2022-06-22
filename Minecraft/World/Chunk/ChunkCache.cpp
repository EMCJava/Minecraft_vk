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
    Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Loading chunk:", chunk.GetCoordinate( ) );

    const auto [ chunkX, chunkZ, chunkY ] = chunk.GetCoordinate( );

    static const std::vector<uint16_t>   blockIndices  = { 0, 1, 2, 2, 3, 0 };
    std::vector<DataType::ColoredVertex> BlockVertices = {
        {{ -0.5f, 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
        { { 0.5f, 0.0f, -0.5f }, { 0.0f, 0.0f, 1.0f }},
        {  { 0.5f, 0.0f, 0.5f }, { 0.0f, 1.0f, 0.0f }},
        { { -0.5f, 0.0f, 0.5f }, { 1.0f, 1.0f, 1.0f }}
    };
    const auto  verticesDataSize = sizeof( DataType::ColoredVertex ) * BlockVertices.size( ) << ( SectionBinaryOffsetLength * 2 );
    const auto  indicesDataSize  = sizeof( uint16_t ) * blockIndices.size( ) << ( SectionBinaryOffsetLength * 2 );
    auto&       api              = MainApplication::GetInstance( ).GetVulkanAPI( );
    const auto& noiseGenerator   = MinecraftServer::GetInstance( ).GetWorld( ).GetHeightNoise( );


    std::unique_ptr<DataType::ColoredVertex[]> chunkVertices = std::make_unique<DataType::ColoredVertex[]>( BlockVertices.size( ) << ( SectionBinaryOffsetLength * 2 ) );

    m_IndexBufferSize                        = blockIndices.size( ) << ( SectionBinaryOffsetLength * 2 );
    std::unique_ptr<uint16_t[]> chunkIndices = std::make_unique<uint16_t[]>( m_IndexBufferSize );

    const auto blockVerticesSize = BlockVertices.size( );
    const auto blockIndicesSize  = blockIndices.size( );
    for ( int i = 0; i < SectionUnitLength; ++i )
    {
        for ( int j = 0; j < SectionUnitLength; ++j )
        {
            const auto height                     = noiseGenerator->GetNoise( ( chunkX << SectionBinaryOffsetLength ) + (float) i, ( chunkZ << SectionBinaryOffsetLength ) + (float) j ) * 10;
            const auto blockCoordinateIndexVertex = ( i * SectionUnitLength + j ) * blockVerticesSize;
            for ( int k = 0; k < blockVerticesSize; ++k )
            {
                assert( blockCoordinateIndexVertex + k < ( BlockVertices.size( ) << ( SectionBinaryOffsetLength * 2 ) ) );
                chunkVertices[ blockCoordinateIndexVertex + k ] = BlockVertices[ k ];
                chunkVertices[ blockCoordinateIndexVertex + k ].pos += glm::vec3( ( chunkX << SectionBinaryOffsetLength ) + i, height, ( chunkZ << SectionBinaryOffsetLength ) + j );
            }

            const auto blockCoordinateIndexIndices = ( i * SectionUnitLength + j ) * blockIndicesSize;
            for ( int k = 0; k < blockIndicesSize; ++k )
            {
                assert( blockCoordinateIndexIndices + k < ( blockIndices.size( ) << ( SectionBinaryOffsetLength * 2 ) ) );
                chunkIndices[ blockCoordinateIndexIndices + k ] = blockIndices[ k ] + ( i * SectionUnitLength + j ) * blockVerticesSize;
            }
        }
    }

    {
        VulkanAPI::VKBufferMeta stagingBuffer;
        vk::BufferCopy          bufferRegion;
        using Usage = vk::BufferUsageFlagBits;

        stagingBuffer.Create( verticesDataSize, Usage::eVertexBuffer | Usage::eTransferSrc, api );
        m_VertexBuffer.Create( verticesDataSize, Usage::eVertexBuffer | Usage::eTransferDst, api,
                               vk::MemoryPropertyFlagBits::eDeviceLocal );
        m_IndexBuffer.Create( verticesDataSize, Usage::eIndexBuffer | Usage::eTransferDst, api,
                              vk::MemoryPropertyFlagBits::eDeviceLocal );

        bufferRegion.setSize( verticesDataSize );
        stagingBuffer.writeBuffer( chunkVertices.get( ), verticesDataSize, api );
        m_VertexBuffer.CopyFromBuffer( stagingBuffer, bufferRegion, api );

        bufferRegion.setSize( indicesDataSize );
        stagingBuffer.writeBuffer( chunkIndices.get( ), indicesDataSize, api );
        m_IndexBuffer.CopyFromBuffer( stagingBuffer, bufferRegion, api );
    }

    // using namespace std::chrono_literals;
    // std::this_thread::sleep_for(1s);
}
