//
// Created by loys on 6/1/22.
//

#ifndef MINECRAFT_VK_MINECRAFT_HPP
#define MINECRAFT_VK_MINECRAFT_HPP

#include <Minecraft/Block/BlockTexture.hpp>
#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>
#include <Minecraft/util/Tickable.hpp>

class Minecraft : public Tickable
    , public Singleton<Minecraft>
{
    std::unique_ptr<BlockTexture> m_BlockTextures;
    std::unique_ptr<MinecraftServer> m_Server;

public:
    Minecraft( ) = default;

    void Tick( float deltaTime );

    void InitServer( );

    void InitTexture( const std::string& folder );

    const auto& GetBlockTextures( ) { return *m_BlockTextures; }
};


#endif   // MINECRAFT_VK_MINECRAFT_HPP
