//
// Created by loyus on 1/1/2023.
//

#include "Entity.hpp"

#include <Minecraft/Application/MainApplication.hpp>
#include <Minecraft/World/Physics/Ray/Raycast.hpp>

// assume x y has the same sign
#define MIN_ABS( x, y ) ( ( x ) > 0 ? ( ( x ) > ( y ) ? ( y ) : ( x ) ) : ( ( x ) > ( y ) ? ( x ) : ( y ) ) )

#define SMALL_DISTANCE   0.0001f
#define SMALL_DISTANCE_2 ( SMALL_DISTANCE * 2 )

inline glm::vec3
PointMove( const glm::vec3& pointPosition, const glm::vec3& maxTravel )
{
    glm::vec3 distance { 0 };

    // for three axis movement
    for ( int i = 0; i < 3; ++i )
    {
        if ( maxTravel[ i ] == 0 ) continue;

        Physics::Ray<FloatTy> ray;
        ray[ i ] = glm::abs( maxTravel[ i ] ) > 1 ? maxTravel[ i ] : ( maxTravel[ i ] > 0 ? 1.f : -1.f );

        using Raycast = Physics::Raycast;

        const auto minectaftPosition = ToMinecraftCoordinate( glm::vec3 { pointPosition } );
        const auto raycast           = Raycast::CastRay( MinecraftServer::GetInstance( ).GetWorld( ), { minectaftPosition[ 0 ], minectaftPosition[ 1 ], minectaftPosition[ 2 ] }, ray );

        if ( raycast.hasSolidHit )
        {
            // not initially inside a block
            if ( raycast.beforeSolidHit != raycast.solidHit )
            {
                const auto hitCoordinate =
                    i == 0 ? std::get<MinecraftCoordinateXIndex>( raycast.solidHit )
                           : ( i == 1 ? std::get<MinecraftCoordinateYIndex>( raycast.solidHit )
                                      : std::get<MinecraftCoordinateZIndex>( raycast.solidHit ) );

                const auto hitDistance = hitCoordinate - pointPosition[ i ] + ( maxTravel[ i ] < 0 ? 1 : 0 );   // block has width, height and length of 1
                distance[ i ]          = MIN_ABS( maxTravel[ i ], hitDistance );

                // avoid stuck in block
                if ( distance[ i ] == hitDistance )
                {
                    const auto absDistance = glm::abs( distance[ i ] );

                    // distance to wall is bigger than SMALL_DISTANCE, leave some space
                    if ( absDistance >= SMALL_DISTANCE_2 )
                        distance[ i ] -= maxTravel[ i ] > 0 ? SMALL_DISTANCE : -SMALL_DISTANCE;
                    else
                        distance[ i ] = 0;

                    // Logger::getInstance( ).LogLine( (double) pointPosition[ i ], (double) distance[ i ] );
                }
            }
        } else
        {
            distance[ i ] = maxTravel[ i ];
        }
    }

    return distance;
}

void
Entity::Tick( float deltaTime )
{
    Tickable::Tick( deltaTime );

    const glm::vec3 entityHitbox { 0.8, 1.9, 0.8 };

    if ( m_UseGravity )
    {
        m_Velocity.y += deltaTime * m_Gravity;

        // "m_AccumulatedYVelocity:", m_AccumulatedYVelocity, " fallDistance: ", fallDistance
        // Logger::getInstance( ).LogLine( "raycast.beforeSolidHit: ", raycast.beforeSolidHit );
    }

    glm::vec3 travel = m_Velocity * deltaTime;

    // for hitbox size
    for ( FloatTy hitX = 0;; hitX += 1 )
    {
        hitX = glm::min( entityHitbox.x, hitX );
        for ( FloatTy hitY = 0;; hitY += 1 )
        {
            hitY = glm::min( entityHitbox.y, hitY );
            for ( FloatTy hitZ = 0;; hitZ += 1 )
            {
                hitZ = glm::min( entityHitbox.z, hitZ );

                const auto pointTravel = PointMove( m_Position + glm::vec3 { hitX, hitY, hitZ }, travel );

                travel.x = MIN_ABS( travel.x, pointTravel.x );
                travel.y = MIN_ABS( travel.y, pointTravel.y );
                travel.z = MIN_ABS( travel.z, pointTravel.z );

                if ( hitZ >= entityHitbox.z ) break;
            }
            if ( hitY >= entityHitbox.y ) break;
        }
        if ( hitX >= entityHitbox.x ) break;
    }

    // Logger::getInstance( ).LogLine( '[', m_Position.x, m_Position.y, m_Position.z, ']', '[', travel.x, travel.y, travel.z, ']' );

    // blocked
    for ( int i = 0; i < 3; ++i )
        if ( travel[ i ] == 0 )
            m_Velocity[ i ] = 0;

    m_Position += travel;
    // Logger::getInstance( ).LogLine( m_Velocity.x, m_Velocity.y, m_Velocity.z, travel.x, travel.y, travel.z );

    // friction
    const auto backupY = m_Velocity.y;
    const auto fpsFactor = 0.0625f / deltaTime;
    m_Velocity -= ( m_Velocity * ( 1 - m_Friction ) ) / fpsFactor;
    m_Velocity.y = backupY; // No friction on vertical axis
}
