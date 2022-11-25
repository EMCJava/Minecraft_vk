//
// Created by loys on 7/26/22.
//

#ifndef MINECRAFT_VK_STRUCTURE_HPP
#define MINECRAFT_VK_STRUCTURE_HPP

#include <cassert>

#include "StructurePieces.hpp"

#include <Minecraft/Block/Block.hpp>
#include <Minecraft/util/MinecraftConstants.hpp>
#include <Minecraft/util/MinecraftType.h>

class Structure : public StructurePieces
{

protected:
    BlockCoordinate m_StartingPosition;

public:
    virtual ~Structure( )                            = default;
    virtual void Generate( class WorldChunk& chunk ) = 0;

    class WorldChunk& GetStartingChunk( class WorldChunk& generatingChunk );

    template <int FirstDim>
    inline void FillLine( class Chunk& chunk, const Block& block, BlockCoordinate begin, CoordinateType EndValue, bool replace = true )
    {
        const int32_t Diff = EndValue - std::get<FirstDim>( begin );
        if ( Diff > 0 )
        {
            for ( int i = 0; i <= Diff; i++ )
            {
                SetBlockWorld( chunk, block, begin, replace );
                std::get<FirstDim>( begin ) += 1;
            }
        } else
        {

            for ( int i = 0; i <= -Diff; i++ )
            {
                SetBlockWorld( chunk, block, begin, replace );
                std::get<FirstDim>( begin ) -= 1;
            }
        }
    }

    template <int FirstDim, int SecDim>
    inline void FillFace( class Chunk& chunk, const Block& block, CoordinateType minF, CoordinateType maxF, CoordinateType minS, CoordinateType maxS, CoordinateType ThirdDimValue, bool replace = true )
    {
        assert( minF <= maxF && minS <= maxS );

        BlockCoordinate currentCoordinate { ThirdDimValue, ThirdDimValue, ThirdDimValue };
        for ( int i = minF; i <= maxF; i++ )
        {
            std::get<FirstDim>( currentCoordinate ) = i;
            for ( int j = minS; j <= maxS; j++ )
            {
                std::get<SecDim>( currentCoordinate ) = j;
                SetBlockWorld( chunk, block, currentCoordinate, replace );
            }
        }
    }

    template <int FirstDim, int SecDim>
    inline void FillFaceHollow( class Chunk& chunk, const Block& block, CoordinateType minF, CoordinateType maxF, CoordinateType minS, CoordinateType maxS, CoordinateType ThirdDimValue, bool replace = true )
    {
        BlockCoordinate currentCoordinate { ThirdDimValue, ThirdDimValue, ThirdDimValue };
        std::get<FirstDim>( currentCoordinate ) = minF;
        std::get<SecDim>( currentCoordinate )   = minS;

        FillLine<FirstDim>( chunk, block, currentCoordinate, maxF - 1, replace );
        ++std::get<SecDim>( currentCoordinate );
        FillLine<SecDim>( chunk, block, currentCoordinate, maxS, replace );

        --std::get<SecDim>( currentCoordinate );
        std::get<FirstDim>( currentCoordinate ) += maxF - minF;
        FillLine<SecDim>( chunk, block, currentCoordinate, maxS - 1, replace );


        std::get<SecDim>( currentCoordinate ) += maxS - minS;
        FillLine<FirstDim>( chunk, block, currentCoordinate, minF, replace );
    }

    void FillCube( class Chunk& chunk, const Block& block, CoordinateType minX, CoordinateType maxX, CoordinateType minY, CoordinateType maxY, CoordinateType minZ, CoordinateType maxZ, bool replace = true );

    void FillCubeHollow( class Chunk& chunk, const Block& block, CoordinateType minX, CoordinateType maxX, CoordinateType minY, CoordinateType maxY, CoordinateType minZ, CoordinateType maxZ, bool replaceInside = true, bool replaceOutside = true );

    bool SetBlockWorld( class Chunk& chunk, const Block& block, const BlockCoordinate& worldCoordinate, bool replace = true );
    bool SetBlock( class Chunk& chunk, const Block& block, const BlockCoordinate& inChunkCoordinate, bool replace = true );

    static constexpr inline auto
    ToHorizontalIndex( const auto& coordinate )
    {
        return ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( coordinate ) ) + GetMinecraftX( coordinate );
    }

    size_t GetObjectSize( ) const;
};

#endif   // MINECRAFT_VK_STRUCTURE_HPP
