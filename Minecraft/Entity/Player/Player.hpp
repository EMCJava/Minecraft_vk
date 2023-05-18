//
// Created by loys on 6/1/22.
//

#ifndef MINECRAFT_VK_PLAYER_HPP
#define MINECRAFT_VK_PLAYER_HPP

#include "Minecraft/Block/Block.hpp"
#include "Minecraft/Entity/CameraEntity.hpp"
#include "Minecraft/World/Physics/Ray/RaycastType.hpp"
#include "Minecraft/util/MinecraftConstants.hpp"
#include "Minecraft/util/MinecraftType.h"

#include "Utility/Animation/AnimationLinear.hpp"

class Player : public CameraEntity
{
    Physics::RaycastResult m_CurrentFrameRaycastResult;

    AnimationLinear SprintAnimationStep = 0.f;

    Block m_BlockHolding = { Stone };

public:
    explicit Player( const EntityCoordinate& coordinate )
        : CameraEntity( coordinate )
    {
        SetEyesPosition( glm::vec3 { 0.4, 1.8, 0.4 } );
        DoRaycast( );
    }

    void                                 DoRaycast( );
    [[nodiscard]] Physics::RaycastResult GetRaycastResult( ) const;

    inline void SetBlockHolding( Block block ) { m_BlockHolding = block; }
    [[nodiscard]] inline Block GetBlockHolding( ) const { return m_BlockHolding; }

    void Tick( float deltaTime );
};


#endif   // MINECRAFT_VK_PLAYER_HPP
