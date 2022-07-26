//
// Created by loys on 7/26/22.
//

#include "StructureTree.hpp"

#include <Minecraft/World/Chunk/Chunk.hpp>

void
StructureTree::Generate( Chunk& chunk )
{
    auto startingIndex = ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( m_StartingPosition ) ) + GetMinecraftX( m_StartingPosition );
    auto heightAtPoint = chunk.GetHeight( startingIndex ) + 1;
    auto treeHeight    = std::min( ChunkMaxHeight - heightAtPoint, 7 );
    for ( int i = 0; i < treeHeight; ++i )
    {
        chunk.SetBlock( startingIndex + ScaleToSecond<1, SectionSurfaceSize>( heightAtPoint + i ), BlockID::AcaciaLog );
    }
}

bool
StructureTree::TryGenerate( Chunk& chunk, std::vector<std::shared_ptr<Structure>>& structureList )
{
    bool           generated  = false;
    MinecraftNoise chunkNoise = chunk.GetChunkNoise( );

    int horizontalMapIndex = 0;
    for ( int k = 0; k < SectionUnitLength; ++k )
        for ( int j = 0; j < SectionUnitLength; ++j, ++horizontalMapIndex )
        {
            if ( chunkNoise.NextUint64( ) % 80 == 0 )
            {
                generated = true;

                auto newStructure                                 = std::make_shared<StructureTree>( );
                GetMinecraftX( newStructure->m_StartingPosition ) = j;
                GetMinecraftZ( newStructure->m_StartingPosition ) = k;

                structureList.emplace_back( std::move( newStructure ) );
            }
        }

    return generated;
}
