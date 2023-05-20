//
// Created by samsa on 9/13/2022.
//

#ifndef MINECRAFT_VK_MINECRAFT_WORLD_BIOME_BIOMESETTINGS_HPP
#define MINECRAFT_VK_MINECRAFT_WORLD_BIOME_BIOMESETTINGS_HPP

#include <Minecraft/World/Generation/Structure/StructureAbandonedHouse.hpp>
#include <Minecraft/World/Generation/Structure/StructureGroup.hpp>
#include <Minecraft/World/Generation/Structure/StructureCave.hpp>
#include <Minecraft/World/Generation/Structure/StructureTree.hpp>

#include <Minecraft/Block/Block.hpp>

struct DefaultBiome {
    using StructuresSettings             = StructureGroup<StructureTree, StructureAbandonedHouse, StructureCave>;
    static constexpr BlockID topBlock    = Grass;
    static constexpr BlockID fillerBlock = Dart;
};


#endif   // MINECRAFT_VK_MINECRAFT_WORLD_BIOME_BIOMESETTINGS_HPP
