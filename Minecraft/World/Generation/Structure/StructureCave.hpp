//
// Created by samsa on 5/19/2023.
//

#ifndef MINECRAFT_VK_STRUCTURECAVE_HPP
#define MINECRAFT_VK_STRUCTURECAVE_HPP

#include "Structure.hpp"
#include <vector>

class WorldChunk;
class StructureCave : public Structure
{
public:
    void Generate( class WorldChunk& chunk ) override;

    static bool TryGenerate( WorldChunk& chunk, std::vector<std::shared_ptr<Structure>>& structureList );

protected:
    /**
     * Generates a larger initial cave node than usual. Called 25% of the time.
     */
    void generateLargeCaveNode( class MinecraftNoise& noise, uint64_t nodeSeed, int chunkX, int chunkZ, WorldChunk& chunk, double par6, double par8, double par10 );

    /**
     * Generates a node in the current cave system recursion tree.
     */

    void generateCaveNode( uint64_t nodeSeed, int chunkX, int chunkZ, WorldChunk& chunk, double par6, double par8, double par10, float par12, float par13, float par14, int from___, int to___, double par17 );

    /**
     * Recursively called by generate() (generate) and optionally by itself.
     */

    void recursiveGenerate( MinecraftNoise& noise, int par2, int par3, int par4, int par5, WorldChunk& chunk );


    bool isOceanBlock( WorldChunk& chunk, int blockIndex, int x, int y, int z, int chunkX, int chunkZ );

private:
    // Determine if the block at the specified location is the top block for the biome, we take into account
    // Vanilla bugs to make sure that we generate the map the same way vanilla does.

    bool isTopBlock( WorldChunk& chunk, int blockIndex, int blockX, int blockY, int blockZ, int chunkX, int chunkZ );

    /**
     * Digs out the current block, default implementation removes stone, filler, and top block
     * Sets the block to lava if y is less then 10, and air other wise.
     * If setting to air, it also checks to see if we've broken the surface and if so
     * tries to make the floor the biome's top block
     *
     * @param data Block data array
     * @param index Pre-calculated index into block data
     * @param blockX local X position
     * @param blockY local Y position
     * @param blockZ local Z position
     * @param chunkX Chunk X position
     * @param chunkZ Chunk Y position
     * @param foundTop True if we've encountered the biome's top block. Ideally if we've broken the surface.
     */
protected:
    void digBlock( WorldChunk& chunk, int blockIndex, int blockX, int blockY, int blockZ, int chunkX, int chunkZ, bool foundTop );
};

#endif   // MINECRAFT_VK_STRUCTURECAVE_HPP
