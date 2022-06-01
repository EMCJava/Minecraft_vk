//
// Created by loys on 6/1/22.
//

#ifndef MINECRAFT_VK_MINECRAFT_HPP
#define MINECRAFT_VK_MINECRAFT_HPP

#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>
#include <Minecraft/util/Tickable.hpp>

class Minecraft : public Tickable
{

    std::unique_ptr<MinecraftServer> m_Server;

public:
    Minecraft() = default;

    void Tick( float deltaTime );

    void InitServer();

};


#endif   // MINECRAFT_VK_MINECRAFT_HPP
