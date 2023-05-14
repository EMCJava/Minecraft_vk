//
// Created by samsa on 8/24/2022.
//

#ifndef MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_STRUCTURE_STRUCTUREABANDONEDHOUSE_HPP
#define MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_STRUCTURE_STRUCTUREABANDONEDHOUSE_HPP

#include "Structure.hpp"

class StructureAbandonedHouse : public Structure
{
public:
    void Generate( class WorldChunk& chunk ) override;

    static bool TryGenerate( class WorldChunk& chunk, std::vector<std::shared_ptr<Structure>>& structureList );
};


#endif   // MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_STRUCTURE_STRUCTUREABANDONEDHOUSE_HPP
