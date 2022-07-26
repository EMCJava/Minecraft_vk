//
// Created by loys on 7/26/22.
//

#ifndef MINECRAFT_VK_CHUNKSTATUS_HPP
#define MINECRAFT_VK_CHUNKSTATUS_HPP

using ChunkStatusTy = uint8_t;
enum ChunkStatus : ChunkStatusTy {

    eEmpty,                // empty chunk
    eStructureStart,       // check if a structure can be generated
    eStructureReference,   // chunk if surrounding chunk has structure that expended to this chunk
    eNoise,                // starts generating chunk, e.g. land scape
    eFeature,              // final decoration, e.g. placing trees / structure
    eFull                  // a completed chunk
};

inline ChunkStatus&
operator++( ChunkStatus& status )
{
    return status = ChunkStatus( ChunkStatusTy( status ) + 1 );
}

#endif   // MINECRAFT_VK_CHUNKSTATUS_HPP
