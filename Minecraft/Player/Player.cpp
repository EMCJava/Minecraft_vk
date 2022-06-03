//
// Created by loys on 6/1/22.
//

#include <Minecraft/Application/MainApplication.hpp>

#include "Player.hpp"

void
Player::Tick( float deltaTime )
{
    Tickable::Tick( deltaTime );

    const auto& deltaMouse = MainApplication::GetInstance( ).GetDeltaMouse( );

    m_Camera.ProcessMouseMovement( deltaMouse );
    m_Camera.ProcessKeyboard( MainApplication::GetInstance( ).GetMovementDelta( ), deltaTime );
}
