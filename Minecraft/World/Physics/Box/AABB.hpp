//
// Created by samsa on 7/31/2022.
//

#ifndef MINECRAFT_VK_MINECRAFT_WORLD_PHYSICS_BOX_AABB_HPP
#define MINECRAFT_VK_MINECRAFT_WORLD_PHYSICS_BOX_AABB_HPP

template <typename Ty = float>
struct TAABB {
    Ty minX, minY, maxX, maxY;

    inline bool IsOverlapping( const TAABB& other ) const
    {
        return operator&( other );
    }

    inline bool operator&( const TAABB& other ) const
    {
        return ( minX <= other.maxX && maxX >= other.minX ) && ( minY <= other.maxY && maxY >= other.minY );
    }
};

using AABB = TAABB<float>;

#endif   // MINECRAFT_VK_MINECRAFT_WORLD_PHYSICS_BOX_AABB_HPP
