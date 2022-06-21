//
// Created by loys on 5/24/22.
//

#include <Minecraft/Application/MainApplication.hpp>

#include <Graphic/Vulkan/VulkanAPI.hpp>

#include <Utility/Logger.hpp>

#include "ChunkCache.hpp"

void
ChunkCache::ResetLoad( )
{
    Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Loading chunk:", chunk.GetCoordinate( ) );

    std::vector<DataType::ColoredVertex> vertices = {
        {{ -0.5f, 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
        { { 0.5f, 0.0f, -0.5f }, { 0.0f, 0.0f, 1.0f }},
        {  { 0.5f, 0.0f, 0.5f }, { 0.0f, 1.0f, 0.0f }},
        { { -0.5f, 0.0f, 0.5f }, { 1.0f, 1.0f, 1.0f }}
    };

    for ( auto& vertex : vertices )
    {
        vertex.pos[ 0 ] += get<0>( chunk.GetCoordinate( ) );
        vertex.pos[ 2 ] += get<1>( chunk.GetCoordinate( ) );
    }

    static const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

    const auto verticesDataSize = sizeof( vertices[ 0 ] ) * vertices.size( );
    const auto indicesDataSize  = sizeof( indices[ 0 ] ) * indices.size( );
    auto&      api              = MainApplication::GetInstance( ).GetVulkanAPI( );

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
        stagingBuffer.writeBuffer( vertices.data( ), verticesDataSize, api );
        m_VertexBuffer.CopyFromBuffer( stagingBuffer, bufferRegion, api );

        bufferRegion.setSize( indicesDataSize );
        stagingBuffer.writeBuffer( indices.data( ), indicesDataSize, api );
        m_IndexBuffer.CopyFromBuffer( stagingBuffer, bufferRegion, api );
    }

    // using namespace std::chrono_literals;
    // std::this_thread::sleep_for(1s);
}
