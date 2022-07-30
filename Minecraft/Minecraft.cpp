//
// Created by loys on 6/1/22.
//

#include "Minecraft.hpp"

void
Minecraft::Tick( float deltaTime )
{
    Tickable::Tick( deltaTime );

    if ( m_Server )
    {
        m_Server->Tick( deltaTime );
    }
}

void
Minecraft::InitServer( )
{
    if ( !m_Server )
    {
        m_Server = std::make_unique<MinecraftServer>( );
        m_Server->InitWorld( );
        m_Server->AddDemoPlayer( MakeMinecraftCoordinate( 0.0f, 200.0f, 0.0f ) );
    }
}

void
Minecraft::InitTexture( const std::string& folder )
{
    m_BlockTextures = std::make_unique<BlockTexture>( folder );
}
