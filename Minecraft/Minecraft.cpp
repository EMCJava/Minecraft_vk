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
        m_Server->AddDemoPlayer( { 0, 200, 0 } );
    }
}

void
Minecraft::InitTexture( const std::string& folder )
{
    m_BlockTextures = std::make_unique<BlockTexture>( folder );
}
