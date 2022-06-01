//
// Created by loys on 6/1/22.
//

#include "MinecraftServer.hpp"

MinecraftServer* MinecraftServer::m_Instance { };

MinecraftServer::MinecraftServer( )
{
    assert( m_Instance == nullptr );
    m_Instance = this;
}

void
MinecraftServer::Tick( float deltaTime )
{
    Tickable::Tick( deltaTime );

    m_MainWorld->Tick( deltaTime );
}

Player
MinecraftServer::GetPlayer( int playerID )
{
    return m_PlayerList[ playerID ];
}

void
MinecraftServer::InitWorld( )
{
    if ( !m_MainWorld )
    {
        m_MainWorld = std::make_unique<MinecraftWorld>( );
    }
}

void
MinecraftServer::AddDemoPlayer( EntityCoordinate coordinate )
{
    m_PlayerList.emplace_back( coordinate );
}
