//
// Created by loys on 6/1/22.
//

#include "Minecraft/Application/MainApplication.hpp"

#include "Player.hpp"


void
Player::Tick( float deltaTime )
{
    const auto preciousPosition = m_Position;
    CameraEntity::Tick( deltaTime );

    const auto& deltaMouse      = MainApplication::GetInstance( ).GetDeltaMouse( );
    const auto& deltaMovement   = MainApplication::GetInstance( ).GetMovementDelta( );
    const float speedMultiplier = MainApplication::GetInstance( ).IsSprint( ) ? 2 : 1;

    {
        if ( deltaMouse.first != 0 || deltaMouse.second != 0 )
            ProcessMouseMovement( deltaMouse );

        if ( deltaMovement.first != 0 || deltaMovement.second != 0 )
            ProcessKeyboard( deltaMovement, deltaTime * speedMultiplier );
    }

    // Flying
    if ( !m_UseGravity )
    {
        // Can't crouch while jumping
        auto verticalMovement = MainApplication::GetInstance( ).IsJumping( ) ? 1 : ( MainApplication::GetInstance( ).IsCrouch( ) ? -1 : 0 );
        ProcessVerticalMovement( verticalMovement, deltaTime * speedMultiplier );
        // m_Camera.ProcessKeyboardHorizontal( MainApplication::GetInstance( ).GetMovementDelta( ), deltaTime );
    } else   // Walking
    {
        static bool previousJump = false;
        bool        currentJump  = MainApplication::GetInstance( ).IsJumping( );
        if ( !previousJump && currentJump )
            m_Velocity.y += 10;

        previousJump = currentJump;
    }

    if ( deltaMouse.first != 0 || deltaMouse.second != 0 || preciousPosition != m_Position ) DoRaycast( );
}

Physics::RaycastResult
Player::GetRaycastResult( ) const
{
    return m_CurrentFrameRaycastResult;
}

void
Player::DoRaycast( )
{
    Physics::Ray<float> ray { Front.x * 20, Front.y * 20, Front.z * 20 };

    const auto minectaftPosition = ToMinecraftCoordinate( m_Position + m_EyePosition );
    m_CurrentFrameRaycastResult  = Physics::Raycast::CastRay( MinecraftServer::GetInstance( ).GetWorld( ), { minectaftPosition[ 0 ], minectaftPosition[ 1 ], minectaftPosition[ 2 ] }, ray );
}
