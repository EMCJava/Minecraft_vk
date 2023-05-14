//
// Created by samsa on 9/13/2022.
//

#ifndef MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_STRUCTURE_STRUCTUREGROUP_HPP
#define MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_STRUCTURE_STRUCTUREGROUP_HPP

#include "Structure.hpp"

template <typename FirstStructure, typename... Structures>
struct StructureGroup {
    static constexpr inline void TryGenerate( class WorldChunk& chunk, std::vector<std::shared_ptr<Structure>>& structureList )
    {
        StructureGroup<FirstStructure>::TryGenerate( chunk, structureList );
        StructureGroup<Structures...>::TryGenerate( chunk, structureList );
    }
};

template <typename FirstStructure>
struct StructureGroup<FirstStructure> {
    static constexpr inline void TryGenerate( class WorldChunk& chunk, std::vector<std::shared_ptr<Structure>>& structureList )
    {
        FirstStructure::TryGenerate( chunk, structureList );
    }
};


#endif   // MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_STRUCTURE_STRUCTUREGROUP_HPP
