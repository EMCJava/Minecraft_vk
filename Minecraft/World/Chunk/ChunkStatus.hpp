//
// Created by loys on 7/26/22.
//

#ifndef MINECRAFT_VK_CHUNKSTATUS_HPP
#define MINECRAFT_VK_CHUNKSTATUS_HPP

using HeightMapStatusTy = uint8_t;
enum HeightMapStatus : HeightMapStatusTy {
    eNoiseHeight,                  // starts generating chunk, e.g. land scape
    eFeatureHeight,                // final decoration, e.g. placing trees / structure
    eFullHeight = eFeatureHeight   // a completed chunk
};


using ChunkStatusTy = uint8_t;
enum ChunkStatus : ChunkStatusTy {

    eEmpty,                // empty chunk
    eStructureStart,       // check if a structure can be generated
    eStructureReference,   // chunk if surrounding chunk has structure that expended to this chunk
    eNoise,                // starts generating chunk, e.g. land scape
    eFeature,              // final decoration, e.g. placing trees / structure
    eFull = eFeature       // a completed chunk
};

inline ChunkStatus&
operator++( ChunkStatus& status )
{
    return status = ChunkStatus( ChunkStatusTy( status ) + 1 );
}

inline ChunkStatus
operator++( ChunkStatus& status, int )
{
    const auto temStatus = status;
    status               = ChunkStatus( ChunkStatusTy( status ) + 1 );

    return temStatus;
}

#endif   // MINECRAFT_VK_CHUNKSTATUS_HPP
