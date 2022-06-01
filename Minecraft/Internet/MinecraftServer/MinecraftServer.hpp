//
// Created by loys on 6/1/22.
//

#ifndef MINECRAFT_VK_MINECRAFTSERVER_HPP
#define MINECRAFT_VK_MINECRAFTSERVER_HPP

#include <Minecraft/Player/Player.hpp>
#include <Minecraft/World/MinecraftWorld.hpp>
#include <Minecraft/util/Tickable.hpp>

#include <vector>

class MinecraftServer : public Tickable
{
private:
    static MinecraftServer* m_Instance;

    std::vector<Player>             m_PlayerList;
    std::unique_ptr<MinecraftWorld> m_MainWorld;

public:
    MinecraftServer( );

    MinecraftServer( const MinecraftServer& ) = delete;
    MinecraftServer( MinecraftServer&& )      = delete;

    MinecraftServer& operator=( const MinecraftServer& ) = delete;
    MinecraftServer& operator=( MinecraftServer&& )      = delete;

    void Tick( float deltaTime );

    void AddDemoPlayer( EntityCoordinate coordinate );
    void InitWorld( );

    Player GetPlayer( int playerID );

    static MinecraftServer& GetInstance( ) { return *m_Instance; }
};


#endif   // MINECRAFT_VK_MINECRAFTSERVER_HPP
