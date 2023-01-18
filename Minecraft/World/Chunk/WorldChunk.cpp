//
// Created by lys on 8/18/22.
//

#include "WorldChunk.hpp"
#include <Minecraft/World/Biome/BiomeSettings.hpp>
#include <Minecraft/World/Generation/Structure/StructureAbandonedHouse.hpp>
#include <Minecraft/World/Generation/Structure/StructureTree.hpp>

#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>

#define GENERATE_DEBUG_CHUNK false

namespace
{
inline void
ResetHeightMap( std::unique_ptr<int32_t[]>& map )
{
    for ( int i = 0; i < SectionSurfaceSize; ++i )
        map[ i ] = -1;
}
}   // namespace

void
WorldChunk::RegenerateChunk( ChunkStatus status )
{
    ResetChunk( );
    UpgradeChunk( status );
}

void
WorldChunk::FillTerrain( const MinecraftNoise& generator )
{
    auto xCoordinate = GetMinecraftX( m_WorldCoordinate );
    auto zCoordinate = GetMinecraftZ( m_WorldCoordinate );

    const auto& noiseOffset = MinecraftServer::GetInstance( ).GetWorld( ).GetTerrainNoiseOffset( );

    m_HeightMap = std::make_unique<int32_t[]>( SectionSurfaceSize );
    ResetHeightMap( m_HeightMap );
    for ( auto& height : m_StatusHeightMap )
        ResetHeightMap( height = std::make_unique<int32_t[]>( SectionSurfaceSize ) );

    m_Blocks = std::make_unique<Block[]>( ChunkVolume );

#if GENERATE_DEBUG_CHUNK

    // the following code are for debug purpose
    for ( int i = 0; i < ChunkVolume; ++i )
        m_Blocks[ i ] = BlockID::Air;

    if ( ManhattanDistance( { 0, 0 } ) == 0 )
    {
        for ( int i = 0; i < 4; i++ )
            for ( int j = 0; j < 5; j++ )
                for ( int k = 0; k < 6; k++ )
                {
                    if ( ( i == 1 && j == 1 ) || ( i == 2 && j == 3 ) )
                    {

                    } else
                    {
                        m_Blocks[ Chunk::GetBlockIndex( MakeMinecraftCoordinate( i, k, j ) ) ] = BlockID::DebugBlock;
                    }
                }
    }


    return;

#endif

    // terrain
    auto*    blocksPtr          = m_Blocks.get( );
    uint32_t horizontalMapIndex = 0;
    for ( int i = 0; i < ChunkMaxHeight; ++i )
    {
        horizontalMapIndex = 0;
        for ( int k = 0; k < SectionUnitLength; ++k )
            for ( int j = 0; j < SectionUnitLength; ++j, ++horizontalMapIndex )
            {
                auto noiseValue = generator.GetNoiseInt( xCoordinate + j, i, zCoordinate + k );
                noiseValue += noiseOffset[ i ];

                assert( blocksPtr + horizontalMapIndex - m_Blocks.get( ) < ChunkVolume );
                blocksPtr[ horizontalMapIndex ] = noiseValue > 0 ? BlockID::Air : BlockID::Stone;
                if ( !blocksPtr[ horizontalMapIndex ].Transparent( ) ) m_HeightMap[ horizontalMapIndex ] = i;
            }

        blocksPtr += SectionSurfaceSize;
    }

    // surface block
    blocksPtr = m_Blocks.get( );
    for ( int i = 0; i < ChunkMaxHeight; ++i )
    {
        horizontalMapIndex = 0;
        for ( int k = 0; k < SectionUnitLength; ++k )
            for ( int j = 0; j < SectionUnitLength; ++j, ++horizontalMapIndex )
            {
                assert( blocksPtr + horizontalMapIndex - m_Blocks.get( ) < ChunkVolume );

                if ( i > m_HeightMap[ horizontalMapIndex ] - 3 )
                {
                    if ( i == m_HeightMap[ horizontalMapIndex ] )
                    {
                        blocksPtr[ horizontalMapIndex ] = BlockID::Grass;

#if SHOW_CHUNK_BARRIER

                        if ( k == 0 || j == 0 )
                        {
                            blocksPtr[ horizontalMapIndex ] = BlockID::BedRock;
                        }

#endif

                    } else if ( i <= m_HeightMap[ horizontalMapIndex ] )
                    {
                        blocksPtr[ horizontalMapIndex ] = BlockID::Dart;
                    }
                }
            }

        blocksPtr += SectionSurfaceSize;
    }
}

void
WorldChunk::FillBedRock( const MinecraftNoise& generator )
{
    auto xCoordinate = GetMinecraftX( m_WorldCoordinate );
    auto zCoordinate = GetMinecraftZ( m_WorldCoordinate );

    auto blackRockHeightMap = std::make_unique<uint32_t[]>( SectionSurfaceSize );

    int horizontalMapIndex = 0;
    for ( int k = 0; k < SectionUnitLength; ++k )
        for ( int j = 0; j < SectionUnitLength; ++j )
        {
            const auto& noiseValue                   = generator.GetNoiseInt( xCoordinate + j, zCoordinate + k ) + 1;
            blackRockHeightMap[ horizontalMapIndex ] = noiseValue * 2 + 1;

            for ( int i = 0; i < blackRockHeightMap[ horizontalMapIndex ]; ++i )
                At( horizontalMapIndex + SectionSurfaceSize * i ) = BlockID ::BedRock;

            ++horizontalMapIndex;
        }
}

void
WorldChunk::UpgradeChunk( ChunkStatus targetStatus )
{
    assert( targetStatus <= ChunkStatus::eFull );

    // already fulfilled
    if ( targetStatus <= m_Status ) return;

    for ( ; !IsChunkStatusAtLeast( targetStatus ); ++m_Status )
    {
        switch ( m_Status )
        {
        case eEmpty:
            if ( !AttemptRunStructureStart( ) ) return;
            break;
        case eStructureStart:
            if ( !AttemptRunStructureReference( ) ) return;
            break;
        case eStructureReference:
            if ( !AttemptRunNoise( ) ) return;
            CopyHeightMapTo( eNoiseHeight );
            break;
        case eNoise:
            if ( !AttemptRunFeature( ) ) return;
            CopyHeightMapTo( eFullHeight );
            break;
        case eFeature: break;
        }
    }
}

bool
WorldChunk::AttemptRunStructureStart( )
{
#if !GENERATE_DEBUG_CHUNK

    DefaultBiome::StructuresSettings::TryGenerate( *this, m_StructureStarts );

#endif

    return true;
}

bool
WorldChunk::AttemptRunStructureReference( )
{
    if ( UpgradeStatusAtLeastInRange( ChunkStatus::eStructureStart, StructureReferenceStatusRange ) ) return false;

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

bool
WorldChunk::AttemptRunNoise( )
{
    FillTerrain( *MinecraftServer::GetInstance( ).GetWorld( ).GetTerrainNoise( ) );

#if !GENERATE_DEBUG_CHUNK

    FillBedRock( *MinecraftServer::GetInstance( ).GetWorld( ).GetBedRockNoise( ) );

#endif

    return true;
}

bool
WorldChunk::AttemptRunFeature( )
{

    if ( UpgradeStatusAtLeastInRange( ChunkStatus::eNoise, 2 ) ) return false;

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

void
WorldChunk::SetCoordinate( const ChunkCoordinate& coordinate )
{
    Chunk::SetCoordinate( coordinate );

    m_ChunkNoise = GenerateChunkNoise( *MinecraftServer::GetInstance( ).GetWorld( ).GetTerrainNoise( ) );
}

bool
WorldChunk::CanRunStructureStart( ) const
{
    return true;
}

bool
WorldChunk::CanRunStructureReference( ) const
{
    return IsSavedChunksStatusAtLeastInRange( ChunkStatus::eStructureStart, StructureReferenceStatusRange );
}

bool
WorldChunk::CanRunNoise( ) const
{
    return true;
}

bool
WorldChunk::CanRunFeature( ) const
{
    return IsSavedChunksStatusAtLeastInRange( ChunkStatus::eNoise, 2 );
}

std::weak_ptr<WorldChunk>&
WorldChunk::GetChunkReference( uint32_t index, const ChunkCoordinate& worldCoordinate )
{
    if ( auto& chunkPtr = m_ChunkReferencesSaves[ index ]; !chunkPtr.expired( ) )
    {
        return chunkPtr;
    }

    return m_ChunkReferencesSaves[ index ] = m_World->GetChunkCacheUnsafe( worldCoordinate );
}

std::weak_ptr<WorldChunk>
WorldChunk::GetChunkReferenceConst( uint32_t index, const ChunkCoordinate& worldCoordinate ) const
{
    if ( auto& chunkPtr = m_ChunkReferencesSaves[ index ]; !chunkPtr.expired( ) )
    {
        return chunkPtr;
    }

    return { };
}

bool
WorldChunk::UpgradeStatusAtLeastInRange( ChunkStatus targetStatus, int range )
{
    assert( range <= ChunkReferenceRange );
    int unitOffset = StructureReferenceStatusRange - range;

    std::vector<ChunkCoordinate> missingChunk;

    {
        for ( int dz = -range; dz <= range; ++dz )
            for ( int dx = -range; dx <= range; ++dx )
            {
                const auto chunkCoordinate = m_Coordinate + MakeMinecraftChunkCoordinate( dx, dz );
                if ( auto chunkCache = GetChunkReference( ( ChunkReferenceRange * ( unitOffset + dz + range ) ) + unitOffset + ( dx + range ), chunkCoordinate );
                     chunkCache.expired( ) || !chunkCache.lock( )->IsChunkStatusAtLeast( targetStatus ) )
                    missingChunk.push_back( chunkCoordinate );
            }
    }

    for ( const auto& chunkCoordinate : missingChunk )
    {
        auto& oldTarget = m_MissingEssentialChunks[ chunkCoordinate ];
        oldTarget       = std::max( oldTarget, (ChunkStatusTy) targetStatus );
    }

    return !missingChunk.empty( );
}

bool
WorldChunk::IsSavedChunksStatusAtLeastInRange( ChunkStatus targetStatus, int range ) const
{
    assert( range <= ChunkReferenceRange );
    int unitOffset = StructureReferenceStatusRange - range;

    // std::lock_guard<std::recursive_mutex> lock( m_World->GetChunkPool( ).GetChunkCacheLock( ) );
    for ( int dz = -range; dz <= range; ++dz )
        for ( int dx = -range; dx <= range; ++dx )
        {
            const auto chunkCoordinate = m_Coordinate + MakeMinecraftChunkCoordinate( dx, dz );
            if ( auto chunkCache = GetChunkReferenceConst( ( ChunkReferenceRange * ( unitOffset + dz + range ) ) + unitOffset + ( dx + range ), chunkCoordinate );
                 !chunkCache.expired( ) && !chunkCache.lock( )->IsChunkStatusAtLeast( targetStatus ) ) return false;
        }

    return true;
}

std::vector<std::weak_ptr<WorldChunk>>
WorldChunk::GetChunkRefInRange( int range )
{
    assert( range <= ChunkReferenceRange );
    int unitOffset = StructureReferenceStatusRange - range;

    std::vector<std::weak_ptr<WorldChunk>> chunks;

    {
        std::lock_guard<std::recursive_mutex> lock( m_World->GetChunkPool( ).GetChunkCacheLock( ) );
        for ( int dz = -range; dz <= range; ++dz )
            for ( int dx = -range; dx <= range; ++dx )
            {
                const auto chunkCoordinate    = m_Coordinate + MakeMinecraftChunkCoordinate( dx, dz );
                const auto chunkRelativeIndex = ( ChunkReferenceRange * ( unitOffset + dz + range ) ) + unitOffset + ( dx + range );
                chunks.push_back( GetChunkReferenceConst( chunkRelativeIndex, chunkCoordinate ) );
            }
    }

    return chunks;
}

CoordinateType
WorldChunk::GetHeight( uint32_t index, HeightMapStatus status ) const
{
    return m_StatusHeightMap[ status ][ index ];
}

bool
WorldChunk::StatusUpgradeAllSatisfied( ) const
{
    assert( m_RequiredStatus <= ChunkStatus::eFull );

    // already fulfilled
    if ( m_RequiredStatus <= m_Status ) return true;

    auto temStatus = m_Status;
    bool satisfied = true;
    for ( ; temStatus < m_RequiredStatus && satisfied; ++temStatus )
    {
        switch ( m_Status )
        {
        case eEmpty: satisfied = CanRunStructureStart( ); break;
        case eStructureStart: satisfied = CanRunStructureReference( ); break;
        case eStructureReference: satisfied = CanRunNoise( ); break;
        case eNoise: satisfied = CanRunFeature( ); break;
        case eFeature: return true;   // ???
        }
    }

    return satisfied;
}

bool
WorldChunk::NextStatusUpgradeSatisfied( ) const
{
    assert( m_RequiredStatus <= ChunkStatus::eFull );

    // already fulfilled
    if ( m_RequiredStatus <= m_Status ) return true;

    switch ( m_Status )
    {
    case eEmpty: return CanRunStructureStart( );
    case eStructureStart: return CanRunStructureReference( );
    case eStructureReference: return CanRunNoise( );
    case eNoise: return CanRunFeature( );
    case eFeature: return true;   // ???
    }

    return false;   // ???
}

size_t
WorldChunk::GetObjectSize( ) const
{

    size_t result = RenderableChunk::GetObjectSize( ) + sizeof( WorldChunk ) + m_RequiredBy.capacity( ) * sizeof( m_RequiredBy[ 0 ] ) + m_StructureStarts.capacity( ) * sizeof( m_StructureStarts[ 0 ] ) + m_StructureReferences.size( ) * sizeof( m_StructureReferences.front( ) );

    for ( const auto& ss : m_StructureStarts )
        result += ss->GetObjectSize( );

    return result;
}
