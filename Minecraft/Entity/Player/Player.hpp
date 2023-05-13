//
// Created by loys on 6/1/22.
//

#ifndef MINECRAFT_VK_PLAYER_HPP
#define MINECRAFT_VK_PLAYER_HPP

#include "Minecraft/Entity/CameraEntity.hpp"
#include "Minecraft/World/Physics/Ray/Raycast.hpp"
#include "Minecraft/util/MinecraftConstants.hpp"
#include "Minecraft/util/MinecraftType.h"

class Player : public CameraEntity
{
    Physics::RaycastResult m_CurrentFrameRaycastResult;

public:
    explicit Player( const EntityCoordinate& coordinate )
        : CameraEntity( coordinate )
    {
        SetEyesPosition( glm::vec3 { 0.4, 1.8, 0.4 } );
        DoRaycast( );
    }

    void                                 DoRaycast( );
    [[nodiscard]] Physics::RaycastResult GetRaycastResult( ) const;

    void Tick( float deltaTime );
};


#endif   // MINECRAFT_VK_PLAYER_HPP