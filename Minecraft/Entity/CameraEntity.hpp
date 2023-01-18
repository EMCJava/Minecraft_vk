//
// Created by loyus on 1/2/2023.
//

#ifndef MINECRAFT_VK_MINECRAFT_ENTITY_CAMERAENTITY_HPP
#define MINECRAFT_VK_MINECRAFT_ENTITY_CAMERAENTITY_HPP

#include "PawnEntity.hpp"

#include "Include/GLM.hpp"

class CameraEntity : public PawnEntity
{

    // camera options
    float MovementSpeed { };
    float ViewSensitivity { };
    float Zoom { };

public:
    explicit CameraEntity( const EntityCoordinate& Coordinate, glm::vec3 position = glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3 up = glm::vec3( 0.0f, 1.0f, 0.0f ), float yaw = glm::radians( -90.0f ), float pitch = 0.0f )
        : PawnEntity( Coordinate, position, up, yaw, pitch )
        , MovementSpeed( 20.5f )
        , ViewSensitivity( 0.005f )
        , Zoom( glm::radians( 104.0f / 2 ) )
    {
        updateCameraVectors( );
    }

    // constructor with scalar values
    CameraEntity( float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch )
        : PawnEntity( posX, posY, posZ, upX, upY, upZ, yaw, pitch )
        , MovementSpeed( 20.5f )
        , ViewSensitivity( 0.005f )
        , Zoom( glm::radians( 104.0f / 2 ) )
    {
        updateCameraVectors( );
    }


    [[nodiscard]] inline const auto& GetFOV( ) const
    {
        return Zoom;
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    // direction [x-dir (right), y-dir(forward)]
    inline void ProcessKeyboard( const std::pair<float, float>& direction, float deltaTime )
    {
        const float velocity = MovementSpeed * deltaTime;
        // m_Position += velocity * ( Front * direction.second + Right * direction.first );
        m_Velocity += velocity * ( Front * direction.second + Right * direction.first );
        UpdateViewMatrix( );
    }

    // Same as ProcessKeyboard, but moves horizontal to the ground
    inline void ProcessKeyboardHorizontal( const std::pair<float, float>& direction, float deltaTime )
    {
        const float velocity = MovementSpeed * deltaTime;
        m_Position += velocity * ( glm::normalize( glm::vec3 { Front.x, 0, Front.z } ) * direction.second + Right * direction.first );
        UpdateViewMatrix( );
    }

    inline void ProcessVerticalMovement( float deltaYMovement, float deltaTime )
    {
        const float velocity = MovementSpeed * deltaTime;
        m_Position.y += velocity * deltaYMovement;
        UpdateViewMatrix( );
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    inline void ProcessMouseMovement( const std::pair<double, double>& offset, bool constrainPitch = true )
    {
        Yaw += offset.first * ViewSensitivity;
        Pitch += offset.second * ViewSensitivity;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if ( constrainPitch ) [[likely]]
        {
            static constexpr auto min = glm::radians( -89.0f );
            static constexpr auto max = glm::radians( 89.0f );
            Pitch                     = std::clamp( Pitch, min, max );
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors( );
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    inline void ProcessMouseScroll( float yoffset )
    {
        Zoom = std::clamp( Zoom - yoffset, 1.0f, 45.0f );
    }

    void Tick( float deltaTime );

private:
    // calculates the front vector from the PawnEntity's (updated) Euler Angles
    inline void updateCameraVectors( )
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos( Yaw ) * cos( Pitch );
        front.y = sin( Pitch );
        front.z = sin( Yaw ) * cos( Pitch );
        Front   = glm::normalize( front );
        // also re-calculate the Right and Up vector
        Right = glm::normalize( glm::cross( Front, WorldUp ) );   // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up    = glm::normalize( glm::cross( Right, Front ) );

        UpdateViewMatrix( );
    }
};


#endif   // MINECRAFT_VK_MINECRAFT_ENTITY_CAMERAENTITY_HPP
