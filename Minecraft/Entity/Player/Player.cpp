//
// Created by loys on 6/1/22.
//

#include "Minecraft/Application/MainApplication.hpp"
#include "Minecraft/World/Physics/Ray/Raycast.hpp"

#include "Player.hpp"


void
Player::Tick( float deltaTime )
{
    const auto preciousPosition = m_Position;
    CameraEntity::Tick( deltaTime );

    const auto& deltaMouse      = MainApplication::GetInstance( ).GetDeltaMouse( );
    const auto& deltaMovement   = MainApplication::GetInstance( ).GetMovementDelta( );
    float       speedMultiplier = 4.f;

    if ( MainApplication::GetInstance( ).IsSprint( ) )
        speedMultiplier *= 1.3f;

    constexpr float desiredAnimationTime = 0.1f;
    SprintAnimationStep.SetDirection( MainApplication::GetInstance( ).IsSprint( ) );
    if ( SprintAnimationStep.Advance( deltaTime / desiredAnimationTime ) )
        SetFOV( SprintAnimationStep.Lerp( defaultZoom, defaultBiggerZoom ) );

    {
        if ( deltaMouse.first != 0 || deltaMouse.second != 0 )
            ProcessMouseMovement( deltaMouse );

        if ( deltaMovement.first != 0 || deltaMovement.second != 0 )
        {
            if ( m_UseGravity )
                ProcessKeyboardHorizontal( deltaMovement, deltaTime * speedMultiplier );
            else
                ProcessKeyboard( deltaMovement, deltaTime * speedMultiplier );
        }
    }

    // Flying
    if ( !m_UseGravity )
    {
        // Can't crouch while jumping
        auto verticalMovement = MainApplication::GetInstance( ).IsJumping( ) ? 1 : ( MainApplication::GetInstance( ).IsCrouch( ) ? -1 : 0 );
        ProcessVerticalMovement( (float) verticalMovement, deltaTime * speedMultiplier );
        // m_Camera.ProcessKeyboardHorizontal( MainApplication::GetInstance( ).GetMovementDelta( ), deltaTime );
    } else   // Walking
    {
        static bool previousJump = false;
        bool        currentJump  = MainApplication::GetInstance( ).IsJumping( );
        if ( !previousJump && currentJump && IsOnGround( ) )
        {
            const auto jumpHeight       = 1.25f;
            const auto verticalVelocity = std::sqrt( -2 * m_Gravity * jumpHeight );
            m_Velocity.y += verticalVelocity;
        }

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

    const auto minecraftPosition = ToMinecraftCoordinate( m_Position + m_EyePosition );
    m_CurrentFrameRaycastResult  = Physics::Raycast::CastRay( MinecraftServer::GetInstance( ).GetWorld( ), { minecraftPosition[ 0 ], minecraftPosition[ 1 ], minecraftPosition[ 2 ] }, ray );
}
