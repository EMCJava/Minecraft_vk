//
// Created by loys on 6/2/22.
//

#ifndef MINECRAFT_VK_PAWNENTITY_HPP
#define MINECRAFT_VK_PAWNENTITY_HPP

#include <algorithm>

#include "Entity.hpp"

#include "Include/GLM.hpp"

class PawnEntity : public Entity
{
public:
    // camera Attributes
    glm::vec3 Front { };
    glm::vec3 Up { };
    glm::vec3 Right { };
    glm::vec3 WorldUp { };
    // euler Angles
    float Yaw { };
    float Pitch { };

    glm::mat4 viewMatrixCache { };

    // Position inside the entity
    glm::vec3 m_EyePosition { 0 };

    // constructor with vectors
    explicit PawnEntity( const EntityCoordinate& Coordinate, glm::vec3 up = glm::vec3( 0.0f, 1.0f, 0.0f ), float yaw = glm::radians( -90.0f ), float pitch = 0.0f )
        : Entity( Coordinate )
        , Front( glm::vec3( 0.0f, 0.0f, -1.0f ) )
    {
        WorldUp = up;
        Yaw     = yaw;
        Pitch   = pitch;
    }
    // constructor with scalar values
    PawnEntity( float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch )
        : Entity( { posX, posY, posZ } )
        , Front( glm::vec3( 0.0f, 0.0f, -1.0f ) )
    {
        WorldUp = glm::vec3( upX, upY, upZ );
        Yaw     = yaw;
        Pitch   = pitch;
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    [[nodiscard]] inline const glm::mat4& GetViewMatrix( ) const
    {
        return viewMatrixCache;
    }

    // updates the view matrix calculated using Euler Angles and the LookAt Matrix
    inline void UpdateViewMatrix( )
    {
        viewMatrixCache = glm::lookAt( m_Position + m_EyePosition, m_Position + m_EyePosition + Front, Up );
    }

    inline void SetEyesPosition( glm::vec3 eyePosition )
    {
        m_EyePosition = eyePosition;
    }
};


#endif   // MINECRAFT_VK_PAWNENTITY_HPP
