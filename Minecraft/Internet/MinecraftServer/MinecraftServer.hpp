//
// Created by loys on 6/1/22.
//

#ifndef MINECRAFT_VK_MINECRAFTSERVER_HPP
#define MINECRAFT_VK_MINECRAFTSERVER_HPP

#include <Utility/Singleton.hpp>

#include <Minecraft/Entity/Player/Player.hpp>
#include <Minecraft/util/Tickable.hpp>

#include <vector>

class MinecraftWorld;
class MinecraftServer : public Tickable
    , public Singleton<MinecraftServer>
{
private:
    std::vector<Player>             m_PlayerList;
    std::unique_ptr<MinecraftWorld> m_MainWorld;

public:
    MinecraftServer( );
    ~MinecraftServer( );

    MinecraftServer( const MinecraftServer& ) = delete;
    MinecraftServer( MinecraftServer&& )      = delete;

    MinecraftServer& operator=( const MinecraftServer& ) = delete;
    MinecraftServer& operator=( MinecraftServer&& )      = delete;

    void Tick( float deltaTime );

    void AddDemoPlayer( const EntityCoordinate& coordinate );
    void InitWorld( );

    Player&         GetPlayer( int playerID );
    MinecraftWorld& GetWorld( ) { return *m_MainWorld; }
};


#endif   // MINECRAFT_VK_MINECRAFTSERVER_HPP
