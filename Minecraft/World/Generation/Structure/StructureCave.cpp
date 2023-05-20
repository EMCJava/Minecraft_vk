//
// Created by samsa on 5/19/2023.
// Algorithm from https://www.javatips.net/api/MoKitchen-master/minecraft/net/minecraft/world/gen/MapGenCaves.java
//

#include "StructureCave.hpp"

#include "Minecraft/World/Chunk/Chunk.hpp"
#include "Minecraft/World/Chunk/WorldChunk.hpp"
#include "Minecraft/World/MinecraftWorld.hpp"
#include "Utility/Logger.hpp"

#include <cmath>
#include <numbers>

using namespace std::numbers;

#pragma message( "Defining range" )
#define range 8

void
StructureCave::Generate( WorldChunk& chunk )
{
    const WorldChunk& startingChunk      = GetStartingChunk( chunk );
    const auto        startingCoordinate = startingChunk.WorldToChunkRelativeCoordinate( m_StartingPosition );
    const auto        startingIndex      = ToHorizontalIndex( startingCoordinate );

    const auto heightAtPoint = startingChunk.GetHeight( startingIndex, eNoiseHeight ) + 1;

    auto chunkNoise = startingChunk.CopyChunkNoise( );

    int  chunkRange = range;
    auto l          = chunkNoise.NextUint64( );
    auto i1         = chunkNoise.NextUint64( );

    for ( int j1 = GetMinecraftX( startingCoordinate ) - chunkRange; j1 <= GetMinecraftX( startingCoordinate ) + chunkRange; ++j1 )
    {
        for ( int k1 = GetMinecraftZ( startingCoordinate ) - chunkRange; k1 <= GetMinecraftZ( startingCoordinate ) + chunkRange; ++k1 )
        {
            auto l1    = j1 * l;
            auto i2    = k1 * i1;
            chunkNoise = startingChunk.CopyChunkNoise( );
            chunkNoise.AlterSeed( { l1, i2 } );
            recursiveGenerate( chunkNoise, j1, k1, GetMinecraftZ( startingCoordinate ), GetMinecraftZ( startingCoordinate ), chunk );
        }
    }
}

bool
StructureCave::TryGenerate( WorldChunk& chunk, std::vector<std::shared_ptr<Structure>>& structureList )
{
    return chunk.CopyChunkNoise( ).NextUint64( 64 ) == 0;
}

void
StructureCave::generateLargeCaveNode( MinecraftNoise& noise, uint64_t nodeSeed, int chunkX, int chunkZ, WorldChunk& chunk, double par6, double par8, double par10 )
{
    generateCaveNode( nodeSeed, chunkX, chunkZ, chunk, par6, par8, par10, noise.NextDouble( ) * 6 + 1, 0.0F, 0.0F, -1, -1, 0.5 );
}

void
StructureCave::generateCaveNode( uint64_t nodeSeed, int chunkX, int chunkZ, WorldChunk& chunk, double par6, double par8, double par10, float par12, float par13, float par14, int from___, int to___, double par17 )
{

    double         d4     = chunkX * 16 + 8;
    double         d5     = chunkZ * 16 + 8;
    float          f3     = 0.0F;
    float          f4     = 0.0F;
    MinecraftNoise random = MinecraftNoise::FromUint64( nodeSeed );

    if ( to___ <= 0 )
    {
        int maxReach = ( range - 1 ) * 16;
        to___        = maxReach - random.NextUint64( maxReach / 4 );
    }

    bool flag = false;

    if ( from___ == -1 )
    {
        from___ = to___ / 2;
        flag    = true;
    }

    int k1 = random.NextUint64( to___ / 2 ) + to___ / 4;

    for ( bool flag1 = random.NextUint64( 6 ) == 0; from___ < to___; ++from___ )
    {
        double d6 = 1.5 + (double) ( sin( (float) from___ * (float) pi / (float) to___ ) * par12 );
        double d7 = d6 * par17;
        float  f5 = cos( par14 );
        float  f6 = sin( par14 );
        par6 += (double) ( cos( par13 ) * f5 );
        par8 += (double) f6;
        par10 += (double) ( sin( par13 ) * f5 );

        if ( flag1 )
        {
            par14 *= 0.92F;
        } else
        {
            par14 *= 0.7F;
        }

        par14 += f4 * 0.1F;
        par13 += f3 * 0.1F;
        f4 *= 0.9F;
        f3 *= 0.75F;
        f4 += ( random.NextDouble( ) - random.NextDouble( ) ) * random.NextDouble( ) * 2.0F;
        f3 += ( random.NextDouble( ) - random.NextDouble( ) ) * random.NextDouble( ) * 4.0F;

        if ( !flag && from___ == k1 && par12 > 1.0F && to___ > 0 )
        {
            generateCaveNode( random.NextUint64( ), chunkX, chunkZ, chunk, par6, par8, par10, random.NextDouble( ) * 0.5F + 0.5F, par13 - ( (float) pi / 2 ), par14 / 3.0F, from___, to___, 1.0 );
            generateCaveNode( random.NextUint64( ), chunkX, chunkZ, chunk, par6, par8, par10, random.NextDouble( ) * 0.5F + 0.5F, par13 + ( (float) pi / 2 ), par14 / 3.0F, from___, to___, 1.0 );
            return;
        }

        if ( flag || random.NextUint64( 4 ) != 0 )
        {
            double d8  = par6 - d4;
            double d9  = par10 - d5;
            double d10 = (double) ( to___ - from___ );
            double d11 = (double) ( par12 + 2.0F + 16.0F );

            if ( d8 * d8 + d9 * d9 - d10 * d10 > d11 * d11 )
            {
                return;
            }

            if ( par6 >= d4 - 16.0 - d6 * 2.0 && par10 >= d5 - 16.0 - d6 * 2.0 && par6 <= d4 + 16.0 + d6 * 2.0 && par10 <= d5 + 16.0 + d6 * 2.0 )
            {
                int beginX = floor( par6 - d6 ) - chunkX * 16 - 1;
                int endX   = floor( par6 + d6 ) - chunkX * 16 + 1;
                int endY   = floor( par8 - d7 ) - 1;
                int beginY = floor( par8 + d7 ) + 1;
                int beginZ = floor( par10 - d6 ) - chunkZ * 16 - 1;
                int endZ   = floor( par10 + d6 ) - chunkZ * 16 + 1;

                if ( beginX < 0 )
                {
                    beginX = 0;
                }

                if ( endX > 16 )
                {
                    endX = 16;
                }

                if ( endY < 1 )
                {
                    endY = 1;
                }

                if ( beginY > 120 )
                {
                    beginY = 120;
                }

                if ( beginZ < 0 )
                {
                    beginZ = 0;
                }

                if ( endZ > 16 )
                {
                    endZ = 16;
                }

                bool reachOcean = false;

                for ( int currentX = beginX; !reachOcean && currentX < endX; ++currentX )
                {
                    for ( int currentZ = beginZ; !reachOcean && currentZ < endZ; ++currentZ )
                    {
                        for ( int height = beginY + 1; !reachOcean && height >= endY - 1; --height )
                        {
                            int blockIndex = ( currentX * 16 + currentZ ) * 128 + height;

                            if ( height >= 0 && height < 128 )
                            {
                                if ( isOceanBlock( chunk, blockIndex, currentX, height, currentZ, chunkX, chunkZ ) )
                                {
                                    reachOcean = true;
                                }

                                if ( height != endY - 1 && currentX != beginX && currentX != endX - 1 && currentZ != beginZ && currentZ != endZ - 1 )
                                {
                                    height = endY;
                                }
                            }
                        }
                    }
                }

                if ( !reachOcean )
                {
                    for ( int currentX = beginX; currentX < endX; ++currentX )
                    {
                        double d12 = ( (double) ( currentX + chunkX * 16 ) + 0.5 - par6 ) / d6;

                        for ( int currentZ = beginZ; currentZ < endZ; ++currentZ )
                        {
                            double d13        = ( (double) ( currentZ + chunkZ * 16 ) + 0.5 - par10 ) / d6;
                            int    blockIndex = ( currentX * 16 + currentZ ) * 128 + beginY;
                            bool   foundTop   = false;

                            if ( d12 * d12 + d13 * d13 < 1.0 )
                            {
                                for ( int k4 = beginY - 1; k4 >= endY; --k4 )
                                {
                                    double d14 = ( (double) k4 + 0.5 - par8 ) / d7;

                                    if ( d14 > -0.7 && d12 * d12 + d14 * d14 + d13 * d13 < 1.0 )
                                    {
                                        if ( isTopBlock( chunk, blockIndex, currentX, k4, currentZ, chunkX, chunkZ ) )
                                        {
                                            foundTop = true;
                                        }

                                        digBlock( chunk, blockIndex, currentX, k4, currentZ, chunkX, chunkZ, foundTop );
                                    }

                                    --blockIndex;
                                }
                            }
                        }
                    }

                    if ( flag )
                    {
                        break;
                    }
                }
            }
        }
    }
}

void
StructureCave::recursiveGenerate( MinecraftNoise& noise, int par2, int par3, int par4, int par5, WorldChunk& chunk )
{
    int i1 = noise.NextUint64( noise.NextUint64( noise.NextUint64( 40 ) + 1 ) + 1 );

    if ( noise.NextUint64( 15 ) != 0 )
    {
        i1 = 0;
    }

    for ( int j1 = 0; j1 < i1; ++j1 )
    {
        double d0 = par2 * 16 + noise.NextUint64( 16 );
        double d1 = noise.NextUint64( noise.NextUint64( 120 ) + 8 );
        double d2 = par3 * 16 + noise.NextUint64( 16 );
        int    k1 = 1;

        if ( noise.NextUint64( 4 ) == 0 )
        {
            generateLargeCaveNode( noise, noise.NextUint64( ), par4, par5, chunk, d0, d1, d2 );
            k1 += noise.NextUint64( 4 );
        }

        for ( int l1 = 0; l1 < k1; ++l1 )
        {
            float f  = noise.NextDouble( ) * (float) pi * 2.0F;
            float f1 = ( noise.NextDouble( ) - 0.5F ) * 2.0F / 8.0F;
            float f2 = noise.NextDouble( ) * 2.0F + noise.NextUint64( );

            if ( noise.NextUint64( 10 ) == 0 )
            {
                f2 *= noise.NextDouble( ) * noise.NextUint64( ) * 3.0F + 1.0F;
            }

            generateCaveNode( noise.NextUint64( ), par4, par5, chunk, d0, d1, d2, f2, f, f1, 0, 0, 1.0 );
        }
    }
}

bool
StructureCave::isOceanBlock( WorldChunk& chunk, int blockIndex, int x, int y, int z, int chunkX, int chunkZ )
{
    return chunk.At( blockIndex ) == Water;
}

bool
StructureCave::isTopBlock( WorldChunk& chunk, int blockIndex, int blockX, int blockY, int blockZ, int chunkX, int chunkZ )
{
    return chunk.At( blockIndex ) == DefaultBiome::topBlock;
}

void
StructureCave::digBlock( WorldChunk& chunk, int blockIndex, int blockX, int blockY, int blockZ, int chunkX, int chunkZ, bool foundTop )
{
    constexpr BlockID top    = DefaultBiome::topBlock;
    constexpr BlockID filler = DefaultBiome::fillerBlock;
    const auto&       block  = chunk.At( blockIndex );

    if ( block == Stone || block == filler || block == top )
    {
        if ( blockY < 10 )
        {
            // Should be lava
            chunk.SetBlock( blockIndex, BlockID::Lava );
        } else
        {
            chunk.At( blockIndex ) = Air;

            // Make a transaction
            if ( foundTop && chunk.At( blockIndex - SectionSurfaceSize ) == filler )
            {
                chunk.SetBlock( blockIndex - SectionSurfaceSize, top );
            }
        }
    }
}
