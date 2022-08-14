//
// Created by samsa on 7/31/2022.
//

#ifndef MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_STRUCTURE_STRUCTUREPIECES_HPP
#define MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_STRUCTURE_STRUCTUREPIECES_HPP

#include <Minecraft/World/Physics/Box/AABB.hpp>

#include <algorithm>
#include <vector>

class StructurePieces
{
    std::vector<AABB> m_PiecesBoundingBox;

public:
    inline void AddPiece( const AABB& boundingBox ) { m_PiecesBoundingBox.push_back( boundingBox ); }

    inline bool IsOverlappingAny( const AABB& boundingBox )
    {
        return std::ranges::any_of( m_PiecesBoundingBox, [ boundingBox ]( auto const& e ) { return e & boundingBox; } );
    }
};


#endif   // MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_STRUCTURE_STRUCTUREPIECES_HPP
