//
// Created by Lo Yu Sum on 5/14/2023.
//

#ifndef MINECRAFT_VK_MINECRAFT_WORLD_PHYSICS_RAY_RAYCASTTYPE_HPP
#define MINECRAFT_VK_MINECRAFT_WORLD_PHYSICS_RAY_RAYCASTTYPE_HPP

#include <Minecraft/util/MinecraftType.h>

#include <string>

namespace Physics
{
template <typename Ty>
struct Ray {
    Ty x { }, y { }, z { };

    inline constexpr auto& operator[]( const size_t& index )
    {
        switch ( index )
        {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        }

        throw std::runtime_error( "Invalid index: " + std::to_string( index ) );
    }
};

struct RaycastResult {

    bool             hasSolidHit = false;
    BlockCoordinate  solidHit { };
    BlockCoordinate  beforeSolidHit { };
    EntityCoordinate solidHitPoint { };

    bool             hasFluidHit = false;
    BlockCoordinate  fluidHit { };
    EntityCoordinate fluidHitPoint { };
};

}   // namespace Physics

#endif   // MINECRAFT_VK_MINECRAFT_WORLD_PHYSICS_RAY_RAYCASTTYPE_HPP
