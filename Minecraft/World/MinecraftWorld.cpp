//
// Created by loys on 5/22/22.
//

#include "MinecraftWorld.hpp"

#include <Include/GlobalConfig.hpp>

MinecraftWorld::MinecraftWorld( )
{
    m_ChunkPool = std::make_unique<ChunkPool>( GlobalConfig::getMinecraftConfigData( )[ "chunk" ][ "loading_thread" ].get<int>( ) );
}
