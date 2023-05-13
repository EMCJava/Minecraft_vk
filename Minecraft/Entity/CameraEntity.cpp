//
// Created by loyus on 1/2/2023.
//

#include "CameraEntity.hpp"
void
CameraEntity::Tick( float deltaTime )
{
    const auto backupPosition = m_Position;
    Entity::Tick( deltaTime );
    if ( backupPosition != m_Position ) UpdateViewMatrix( );
}
