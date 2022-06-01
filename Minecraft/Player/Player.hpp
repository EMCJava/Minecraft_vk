//
// Created by loys on 6/1/22.
//

#ifndef MINECRAFT_VK_PLAYER_HPP
#define MINECRAFT_VK_PLAYER_HPP

#include <Minecraft/util/MinecraftType.h>

class Player
{
    EntityCoordinate m_Coordinate;

public:
    explicit Player( EntityCoordinate coordinate )
    {
        SetCoordinate( coordinate );
    }

    void SetCoordinate( EntityCoordinate coordinate ) { m_Coordinate = coordinate; };

    EntityCoordinate GetCoordinate( ) { return m_Coordinate; };
    BlockCoordinate  GetIntCoordinate( ) { return { std::get<0>( m_Coordinate ), std::get<1>( m_Coordinate ), std::get<2>( m_Coordinate ) }; };
};


#endif   // MINECRAFT_VK_PLAYER_HPP
