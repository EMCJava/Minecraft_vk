//
// Created by loys on 7/26/22.
//

#ifndef MINECRAFT_VK_STRUCTURE_HPP
#define MINECRAFT_VK_STRUCTURE_HPP

#include "StructurePieces.hpp"

#include <Minecraft/util/MinecraftType.h>

class Structure : public StructurePieces
{

protected:
    BlockCoordinate m_StartingPosition;

public:
    virtual ~Structure( )                                 = default;
    virtual void Generate( class WorldChunk& chunk ) = 0;
};

#endif   // MINECRAFT_VK_STRUCTURE_HPP
