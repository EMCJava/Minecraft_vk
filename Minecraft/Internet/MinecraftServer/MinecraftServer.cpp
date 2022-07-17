//
// Created by loys on 6/1/22.
//

#include "MinecraftServer.hpp"

void
MinecraftServer::Tick( float deltaTime )
{
    Tickable::Tick( deltaTime );

    for ( auto& player : m_PlayerList )
        player.Tick( deltaTime );
    m_MainWorld->Tick( deltaTime );
}

Player&
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
MinecraftServer::AddDemoPlayer( const EntityCoordinate& coordinate )
{
    m_PlayerList.emplace_back( coordinate );
}
