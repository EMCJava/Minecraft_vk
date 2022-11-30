//
// Created by samsa on 9/13/2022.
//

#ifndef MINECRAFT_VK_MINECRAFT_WORLD_BIOME_BIOMESETTINGS_HPP
#define MINECRAFT_VK_MINECRAFT_WORLD_BIOME_BIOMESETTINGS_HPP

#include <Minecraft/World/Generation/Structure/StructureGroup.hpp>
#include <Minecraft/World/Generation/Structure/StructureTree.hpp>
#include <Minecraft/World/Generation/Structure/StructureAbandonedHouse.hpp>

struct DefaultBiome{
using StructuresSettings = StructureGroup<StructureTree, StructureAbandonedHouse>;
};


#endif   // MINECRAFT_VK_MINECRAFT_WORLD_BIOME_BIOMESETTINGS_HPP
