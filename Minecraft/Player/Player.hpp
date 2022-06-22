//
// Created by loys on 6/1/22.
//

#ifndef MINECRAFT_VK_PLAYER_HPP
#define MINECRAFT_VK_PLAYER_HPP

#include <Minecraft/Application/Camera/Camera.hpp>
#include <Minecraft/util/MinecraftConstants.hpp>
#include <Minecraft/util/MinecraftType.h>
#include <Minecraft/util/Tickable.hpp>

class Player : Tickable
{
    Camera           m_Camera { glm::vec3( 0.0f, 2.0f, 0.0f ) };
    EntityCoordinate m_Coordinate;

public:
    explicit Player( EntityCoordinate coordinate )
    {
        SetCoordinate( coordinate );
    }

    // returns the player's view matrix
    [[nodiscard]] inline const glm::mat4& GetViewMatrix( ) const
    {
        return m_Camera.GetViewMatrix( );
    }

    void SetCoordinate( EntityCoordinate coordinate ) { m_Coordinate = coordinate; };

    EntityCoordinate GetCoordinate( ) { return m_Coordinate; };
    BlockCoordinate  GetIntCoordinate( ) { return { std::get<0>( m_Coordinate ), std::get<1>( m_Coordinate ), std::get<2>( m_Coordinate ) }; };
    BlockCoordinate  GetChunkCoordinate( ) { return { (int) std::get<0>( m_Coordinate ) >> SectionBinaryOffsetLength, (int) std::get<1>( m_Coordinate ) >> SectionBinaryOffsetLength, (int) std::get<2>( m_Coordinate ) >> SectionBinaryOffsetLength }; };

    void Tick( float deltaTime );
};


#endif   // MINECRAFT_VK_PLAYER_HPP
