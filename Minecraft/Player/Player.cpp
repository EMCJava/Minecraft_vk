//
// Created by loys on 6/1/22.
//

#include <Minecraft/Application/MainApplication.hpp>

#include "Player.hpp"


void
Player::Tick( float deltaTime )
{
    Tickable::Tick( deltaTime );

    const auto& deltaMouse    = MainApplication::GetInstance( ).GetDeltaMouse( );
    const auto& deltaMovement = MainApplication::GetInstance( ).GetMovementDelta( );

    m_Camera.ProcessMouseMovement( deltaMouse );
    m_Camera.ProcessKeyboard( deltaMovement, deltaTime );
    // m_Camera.ProcessKeyboardHorizontal( MainApplication::GetInstance( ).GetMovementDelta( ), deltaTime );

    m_Coordinate = MakeMinecraftCoordinate<EntityCoordinate>( m_Camera.Position.x, m_Camera.Position.y, m_Camera.Position.z );

    if ( deltaMouse.first != 0 || deltaMouse.second != 0 || deltaMovement.first != 0 || deltaMovement.second != 0 ) DoRaycast( );
}

Physics::RaycastResult
Player::GetRaycastResult( ) const
{
    return m_CurrentFrameRaycastResult;
}

void
Player::DoRaycast( )
{
    Physics::Ray<float> ray { m_Camera.Front.x * 20, m_Camera.Front.y * 20, m_Camera.Front.z * 20 };
    m_CurrentFrameRaycastResult = Physics::Raycast::CastRay( MinecraftServer::GetInstance( ).GetWorld( ), m_Coordinate, ray );
}
