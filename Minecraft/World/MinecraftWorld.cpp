//
// Created by loys on 5/22/22.
//

#include "MinecraftWorld.hpp"

#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>

#include <Include/GlobalConfig.hpp>

MinecraftWorld::MinecraftWorld( )
{
    m_ChunkPool         = std::make_unique<ChunkPool>( GlobalConfig::getMinecraftConfigData( )[ "chunk" ][ "loading_thread" ].get<int>( ) );
    m_ChunkLoadingRange = GlobalConfig::getMinecraftConfigData( )[ "chunk" ][ "chunk_loading_range" ].get<CoordinateType>( );
    m_ChunkPool->SetValidRange( m_ChunkLoadingRange );
    m_ChunkPool->StartThread();
}

void
MinecraftWorld::IntroduceChunkInRange( ChunkCoordinate centre, int32_t radius )
{
    m_ChunkPool->SetCentre( centre );
    for ( int i = -radius; i <= radius; ++i )
        for ( int j = -radius; j <= radius; ++j )
            if ( std::abs( i ) + std::abs( j ) < radius )
                m_ChunkPool->AddCoordinate( centre + MakeCoordinate( i, j, 0 ) );
}

void
MinecraftWorld::Tick( float deltaTime )
{
    Tickable::Tick( deltaTime );

    m_TimeSinceChunkLoad += deltaTime;
    if ( m_TimeSinceChunkLoad > 0.2f )
    {
        IntroduceChunkInRange( MinecraftServer::GetInstance().GetPlayer(0).GetIntCoordinate(), m_ChunkLoadingRange );
        m_TimeSinceChunkLoad = 0;
    }
}