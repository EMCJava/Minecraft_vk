//
// Created by loyus on 1/1/2023.
//

#ifndef MINECRAFT_VK_MINECRAFT_ENTITY_ENTITY_HPP
#define MINECRAFT_VK_MINECRAFT_ENTITY_ENTITY_HPP

#include "Minecraft/util/MinecraftConstants.hpp"
#include "Minecraft/util/MinecraftType.h"
#include "Minecraft/util/Tickable.hpp"

#include "Include/GLM.hpp"

class Entity : public Tickable
{
protected:
    FloatTy   m_Friction = 0.0001f;
    glm::vec3 m_Position, m_Velocity { };

    FloatTy m_Gravity    = -18.f;
    bool    m_UseGravity = true;

public:
    explicit Entity( const EntityCoordinate& coordinate )
    {
        SetCoordinate( coordinate );
    }

    inline void SetCoordinate( const EntityCoordinate& coordinate )
    {
        m_Position[ 0 ] = std::get<0>( coordinate );
        m_Position[ 1 ] = std::get<1>( coordinate );
        m_Position[ 2 ] = std::get<2>( coordinate );

        ToCartesianCoordinate( m_Position );
    }

    inline EntityCoordinate GetCoordinate( )
    {
        return MakeMinecraftCoordinate<EntityCoordinate>(
            m_Position[ 0 ], m_Position[ 1 ], m_Position[ 2 ] );
    }

    inline BlockCoordinate GetIntCoordinate( )
    {
        return MakeMinecraftCoordinate<BlockCoordinate>(
            m_Position[ 0 ], m_Position[ 1 ], m_Position[ 2 ] );
    }

    inline ChunkCoordinate GetChunkCoordinate( )
    {
        return {
            (int) m_Position.x >> SectionUnitLengthBinaryOffset,
            (int) m_Position.z >> SectionUnitLengthBinaryOffset };
    }

    inline void SetAffectedByGravity( bool affected = true ) { m_UseGravity = affected; }
    inline void SetGravity( FloatTy gravity ) { m_Gravity = gravity; }

    void Tick( float deltaTime );
};


#endif   // MINECRAFT_VK_MINECRAFT_ENTITY_ENTITY_HPP
