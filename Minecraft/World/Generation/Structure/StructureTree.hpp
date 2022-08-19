//
// Created by loys on 7/26/22.
//

#ifndef MINECRAFT_VK_STRUCTURETREE_HPP
#define MINECRAFT_VK_STRUCTURETREE_HPP

#include "Minecraft/World/Chunk/Chunk.hpp"
#include "Structure.hpp"

class StructureTree : public Structure
{
public:
    void Generate( class WorldChunk& chunk ) override;

    static bool TryGenerate( class WorldChunk& chunk, std::vector<std::shared_ptr<Structure>>& structureList );
};


#endif   // MINECRAFT_VK_STRUCTURETREE_HPP
