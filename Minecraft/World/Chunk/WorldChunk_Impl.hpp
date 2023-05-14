//
// Created by lys on 8/18/22.
//

#ifndef MINECRAFT_VK_WORLDCHUNK_IMPL_HPP
#define MINECRAFT_VK_WORLDCHUNK_IMPL_HPP

//#include <Minecraft/World/MinecraftWorld.hpp>
//#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>
//#include <Minecraft/World/Biome/BiomeSettings.hpp>

template <>
inline bool
WorldChunk::StatusCompletable<eStructureStart>( ) const
{
    return true;
}

template <>
inline bool
WorldChunk::StatusCompletable<eStructureReference>( ) const
{
    return IsSavedChunksStatusAtLeastInRange( ChunkStatus::eStructureStart, StructureReferenceStatusRange );
}

template <>
inline bool
WorldChunk::StatusCompletable<eNoise>( ) const
{
    return true;
}

template <>
inline bool
WorldChunk::StatusCompletable<eFeature>( ) const
{
    return IsSavedChunksStatusAtLeastInRange( ChunkStatus::eNoise, 2 );
}

template <>
inline bool
WorldChunk::AttemptCompleteStatus<eStructureStart>( )
{
#if !GENERATE_DEBUG_CHUNK

    assert( false );
    // DefaultBiome::StructuresSettings::TryGenerate( *this, m_StructureStarts );

#endif

    return true;
}

template <>
inline bool
WorldChunk::AttemptCompleteStatus<eStructureReference>( )
{
    if ( !UpgradeStatusAtLeastInRange( ChunkStatus::eStructureStart, StructureReferenceStatusRange ) ) return false;

    {
        // chunk cache will not be modified until all thread has finished, mutex can be avoided
        // std::lock_guard<std::recursive_mutex> lock( m_World->GetChunkPool( ).GetChunkCacheLock( ) );
        for ( int index = 0, dx = -StructureReferenceStatusRange; dx <= StructureReferenceStatusRange; ++dx )
        {
            for ( int dz = -StructureReferenceStatusRange; dz <= StructureReferenceStatusRange; ++dz, ++index )
            {
                const auto                  worldCoordinate = m_Coordinate + MakeMinecraftChunkCoordinate( dx, dz );
                const auto                  weakChunkCache  = GetChunkReference( index, worldCoordinate );
                std::shared_ptr<WorldChunk> chunkCache      = weakChunkCache.lock( );

                const auto& chunkReferenceStarts = chunkCache->GetStructureStarts( );
                // m_StructureReferences.reserve( m_StructureReferences.size( ) + chunkReferenceStarts.size( ) );
                for ( const auto& chunkReferenceStart : chunkReferenceStarts )
                {
                    if ( chunkReferenceStart->IsOverlappingAny( *this ) )
                    {
                        m_StructureReferences.emplace_back( chunkReferenceStart );
                    }
                }
            }
        }
    }

    return true;
}

template <>
inline bool
WorldChunk::AttemptCompleteStatus<eNoise>( )
{
    assert( false );

//    FillTerrain( *MinecraftServer::GetInstance( ).GetWorld( ).GetTerrainNoise( ) );
//
//#if !GENERATE_DEBUG_CHUNK
//
//    FillBedRock( *MinecraftServer::GetInstance( ).GetWorld( ).GetBedRockNoise( ) );
//
//#endif

    return true;
}

template <>
inline bool
WorldChunk::AttemptCompleteStatus<eFeature>( )
{
    if ( !UpgradeStatusAtLeastInRange( ChunkStatus::eNoise, 2 ) ) return false;

    for ( auto& ss : m_StructureStarts )
    {
        ss->Generate( *this );
    }

    for ( auto& ss : m_StructureReferences )
    {
        ss.lock( )->Generate( *this );
    }

    return true;
}


inline bool
WorldChunk::UpgradeSatisfied( ChunkStatus status ) const
{
    switch ( m_Status )
    {
    case eEmpty: return StatusCompletable<eStructureStart>( );
    case eStructureStart: return StatusCompletable<eStructureReference>( );
    case eStructureReference: return StatusCompletable<eNoise>( );
    case eNoise: return StatusCompletable<eFeature>( );
    case eFeature: return true;   // ???
    }

    throw std::runtime_error( "Unknown chunk status: " + std::to_string( status ) );
}

#endif //MINECRAFT_VK_WORLDCHUNK_IMPL_HPP