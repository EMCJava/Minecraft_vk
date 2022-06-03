//
// Created by loys on 6/2/22.
//

#ifndef MINECRAFT_VK_CAMERA_HPP
#define MINECRAFT_VK_CAMERA_HPP

#include <algorithm>

#include <Include/GLM.hpp>

class Camera
{
public:
    // camera Attributes
    glm::vec3 Position { };
    glm::vec3 Front { };
    glm::vec3 Up { };
    glm::vec3 Right { };
    glm::vec3 WorldUp { };
    // euler Angles
    float Yaw { };
    float Pitch { };
    // camera options
    float MovementSpeed { };
    float MouseSensitivity { };
    float Zoom { };

    glm::mat4 viewMatrixCache { };

    // constructor with vectors
    explicit Camera( glm::vec3 position = glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3 up = glm::vec3( 0.0f, 1.0f, 0.0f ), float yaw = glm::radians( -90.0f ), float pitch = 0.0f )
        : Front( glm::vec3( 0.0f, 0.0f, -1.0f ) )
        , MovementSpeed( 2.5f )
        , MouseSensitivity( 0.001f )
        , Zoom( 45.0f )
    {
        Position = position;
        WorldUp  = up;
        Yaw      = yaw;
        Pitch    = pitch;
        updateCameraVectors( );
    }
    // constructor with scalar values
    Camera( float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch )
        : Front( glm::vec3( 0.0f, 0.0f, -1.0f ) )
        , MovementSpeed( 2.5f )
        , MouseSensitivity( 0.001f )
        , Zoom( 45.0f )
    {
        Position = glm::vec3( posX, posY, posZ );
        WorldUp  = glm::vec3( upX, upY, upZ );
        Yaw      = yaw;
        Pitch    = pitch;
        updateCameraVectors( );
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    [[nodiscard]] inline const glm::mat4& GetViewMatrix( ) const
    {
        return viewMatrixCache;
    }

    // updates the view matrix calculated using Euler Angles and the LookAt Matrix
    inline void UpdateViewMatrix( )
    {
        viewMatrixCache = glm::lookAt( Position, Position + Front, Up );
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    // direction [x-dir (right), y-dir(forward)]
    inline void ProcessKeyboard( const std::pair<float, float>& direction, float deltaTime )
    {
        const float velocity = MovementSpeed * deltaTime;
        Position += velocity * ( Front * direction.second + Right * direction.first );
        UpdateViewMatrix( );
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    inline void ProcessMouseMovement( const std::pair<double, double>& offset, bool constrainPitch = true )
    {
        Yaw += offset.first * MouseSensitivity;
        Pitch += offset.second * MouseSensitivity;

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

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
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


#endif   // MINECRAFT_VK_CAMERA_HPP
