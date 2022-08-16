//
// Created by samsa on 8/15/2022.
//

#include "Structure.hpp"

#include <Minecraft/World/Chunk/Chunk.hpp>

bool
Structure::SetBlock( Chunk& chunk, const BlockCoordinate& worldCoordinate, const Block& block )
{
    return chunk.SetBlockAtWorldCoordinate( worldCoordinate, block );
}
