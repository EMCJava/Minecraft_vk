//
// Created by samsa on 7/14/2022.
//

#ifndef MINECRAFT_VK_MINECRAFT_PHYSICS_RAY_RAYCAST_HPP
#define MINECRAFT_VK_MINECRAFT_PHYSICS_RAY_RAYCAST_HPP

#include <Utility/Timer.hpp>
#include <Minecraft/World/MinecraftWorld.hpp>

#define GET_STEP( X ) ( X ) > 0 ? 1 : ( ( X ) == 0 ? 0 : -1 )

namespace Physics
{
template <typename Ty>
struct Ray {
    Ty x, y, z;
};

struct RaycastResult {

    bool             hitSolid = false;
    BlockCoordinate  solidHit { };
    EntityCoordinate solidHitPoint { };

    bool             hitFluid = false;
    BlockCoordinate  fluidHit { };
    EntityCoordinate fluidHitPoint { };
};

class Raycast
{

    struct RaycastPathLogger {

        std::vector<BlockCoordinate> path;

        void AddCoordinate( const BlockCoordinate& coordinate )
        {
            path.push_back( coordinate );
        }

        ~RaycastPathLogger( )
        {
            std::stringstream ss;
            ss << '[';
            for ( const auto& coor : path )
                ss << coor << ",";
            ss << ']' << std::flush;
            Logger::getInstance( ).LogLine( ss.str( ) );
        }
    };

public:

    template <typename Ty>
    static inline RaycastResult CastRay( MinecraftWorld& world, const EntityCoordinate& startingPosition, const Ray<Ty>& ray )
    {
        static constexpr Ty maxStep = 10000000;
        // RaycastPathLogger   pathLog;

        // Timer timer;

        BlockCoordinate currentCoordinate = { std::floor( std::get<0>( startingPosition ) ), std::floor( std::get<1>( startingPosition ) ), std::floor( std::get<2>( startingPosition ) ) };
        CoordinateType  stepX = GET_STEP( ray.x ), stepY = GET_STEP( ray.y ), stepZ = GET_STEP( ray.z );
        // CoordinateType  justOutX = stepX + ray.x + GetMinecraftX( startingPosition ), justOutY = stepY + ray.y + GetMinecraftY( startingPosition ), justOutZ = stepZ + ray.z + GetMinecraftZ( startingPosition );
        Ty              tMaxX, tMaxY, tMaxZ, tDeltaX, tDeltaY, tDeltaZ;

        /*
         *
         * X
         *
         * */
        if ( stepX != 0 )
            tDeltaX = std::min( Ty( stepX ) / ray.x, maxStep );
        else
            tDeltaX = maxStep;

        if ( stepX > 0 )
            tMaxX = tDeltaX * ( 1 - GetMinecraftX( std::as_const( startingPosition ) ) + std::floor( GetMinecraftX( std::as_const( startingPosition ) ) ) );
        else
            tMaxX = tDeltaX * ( GetMinecraftX( std::as_const( startingPosition ) ) - std::floor( GetMinecraftX( std::as_const( startingPosition ) ) ) );

        /*
         *
         * Y
         *
         * */
        if ( stepY != 0 )
            tDeltaY = std::min( Ty( stepY ) / ray.y, maxStep );
        else
            tDeltaY = maxStep;

        if ( stepY > 0 )
            tMaxY = tDeltaY * ( 1 - GetMinecraftY( std::as_const( startingPosition ) ) + std::floor( GetMinecraftY( std::as_const( startingPosition ) ) ) );
        else
            tMaxY = tDeltaY * ( GetMinecraftY( std::as_const( startingPosition ) ) - std::floor( GetMinecraftY( std::as_const( startingPosition ) ) ) );


        /*
         *
         * Z
         *
         * */
        if ( stepZ != 0 )
            tDeltaZ = std::min( Ty( stepZ ) / ray.z, maxStep );
        else
            tDeltaZ = maxStep;

        if ( stepZ > 0 )
            tMaxZ = tDeltaZ * ( 1 - GetMinecraftZ( std::as_const( startingPosition ) ) + std::floor( GetMinecraftZ( std::as_const( startingPosition ) ) ) );
        else
            tMaxZ = tDeltaZ * ( GetMinecraftZ( std::as_const( startingPosition ) ) - std::floor( GetMinecraftZ( std::as_const( startingPosition ) ) ) );

        Block* blockPtr;
        while ( true )
        {
            // pathLog.AddCoordinate( currentCoordinate );

            blockPtr = world.GetBlock( currentCoordinate );
            if ( blockPtr != nullptr && !blockPtr->Transparent( ) ) break;

            if ( tMaxX < tMaxY )
            {
                if ( tMaxX < tMaxZ )
                {
                    GetMinecraftX( currentCoordinate ) += stepX;
                    // if ( ( GetMinecraftX( currentCoordinate ) += stepX ) == justOutX )
                    //     return { }; /* outside grid */
                    tMaxX += tDeltaX;
                } else
                {
                    GetMinecraftZ( currentCoordinate ) += stepZ;
                    // if ( ( GetMinecraftZ( currentCoordinate ) += stepZ ) == justOutZ )
                    //     return { }; /* outside grid */
                    tMaxZ += tDeltaZ;
                }
            } else
            {
                if ( tMaxY < tMaxZ )
                {
                    GetMinecraftY( currentCoordinate ) += stepY;
                    // if ( ( GetMinecraftY( currentCoordinate ) += stepY ) == justOutY )
                    //     return { }; /* outside grid */
                    tMaxY += tDeltaY;
                } else
                {
                    GetMinecraftZ( currentCoordinate ) += stepZ;
                    // if ( ( GetMinecraftZ( currentCoordinate ) += stepZ ) == justOutZ )
                    //     return { }; /* outside grid */
                    tMaxZ += tDeltaZ;
                }
            }

            if ( tMaxX > 1 && tMaxY > 1 && tMaxZ > 1 ) return { }; /* outside grid */
        }

        return { true, currentCoordinate };
    }
};

}   // namespace Physics

#endif   // MINECRAFT_VK_MINECRAFT_PHYSICS_RAY_RAYCAST_HPP
