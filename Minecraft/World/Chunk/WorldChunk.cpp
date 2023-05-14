//
// Created by lys on 8/18/22.
//

#include "WorldChunk.hpp"

#include <Minecraft/World/MinecraftWorld.hpp>
#include <Minecraft/World/Generation/Structure/StructureAbandonedHouse.hpp>
#include <Minecraft/World/Generation/Structure/StructureTree.hpp>

#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>

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

    auto blackRockHeightMap = std::make_unique<int[]>( SectionSurfaceSize );

    int horizontalMapIndex = 0;
    for ( int k = 0; k < SectionUnitLength; ++k )
        for ( int j = 0; j < SectionUnitLength; ++j )
        {
            // From 0 to 2
            const auto& noiseValue                   = generator.GetNoiseInt( xCoordinate + j, zCoordinate + k ) + 1;
            blackRockHeightMap[ horizontalMapIndex ] = static_cast<int>( noiseValue * 2 + 1 );

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
            if ( !AttemptCompleteStatus<eStructureStart>( ) ) return;
            break;
        case eStructureStart:
            if ( !AttemptCompleteStatus<eStructureReference>( ) ) return;
            break;
        case eStructureReference:
            if ( !AttemptCompleteStatus<eNoise>( ) ) return;
            CopyHeightMapTo( eNoiseHeight );
            break;
        case eNoise:
            if ( !AttemptCompleteStatus<eFeature>( ) ) return;
            CopyHeightMapTo( eFullHeight );
            break;
        case eFeature: break;
        }
    }
}

void
WorldChunk::SetCoordinate( const ChunkCoordinate& coordinate )
{
    Chunk::SetCoordinate( coordinate );

    m_ChunkNoise = GenerateChunkNoise( *MinecraftServer::GetInstance( ).GetWorld( ).GetTerrainNoise( ) );
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
WorldChunk::TryGetChunkReference( uint32_t index ) const
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

    std::vector<ChunkCoordinate> missingChunk;

    {
        for ( int dz = -range; dz <= range; ++dz )
            for ( int dx = -range; dx <= range; ++dx )
            {
                const auto chunkCoordinate = m_Coordinate + MakeMinecraftChunkCoordinate( dx, dz );
                if ( auto chunkCache = GetChunkReference( GetRefIndexFromCenter( dx, dz ), chunkCoordinate );
                     chunkCache.expired( ) || !chunkCache.lock( )->IsChunkStatusAtLeast( targetStatus ) )
                    missingChunk.push_back( chunkCoordinate );
            }
    }

    for ( const auto& chunkCoordinate : missingChunk )
    {
        auto& oldTarget = m_MissingEssentialChunks[ chunkCoordinate ];
        oldTarget       = std::max( oldTarget, (ChunkStatusTy) targetStatus );
    }

    return missingChunk.empty( );
}

bool
WorldChunk::IsSavedChunksStatusAtLeastInRange( ChunkStatus targetStatus, int range ) const
{
    assert( range <= ChunkReferenceRange );

    for ( int dz = -range; dz <= range; ++dz )
        for ( int dx = -range; dx <= range; ++dx )
            if ( auto chunkCache = TryGetChunkReference( GetRefIndexFromCenter( dx, dz ) );
                 !chunkCache.expired( ) && !chunkCache.lock( )->IsChunkStatusAtLeast( targetStatus ) ) return false;


    return true;
}

std::vector<std::weak_ptr<WorldChunk>>
WorldChunk::GetChunkRefInRange( int range ) const
{
    assert( range <= ChunkReferenceRange );

    std::vector<std::weak_ptr<WorldChunk>> chunks;
    for ( int dz = -range; dz <= range; ++dz )
        for ( int dx = -range; dx <= range; ++dx )
            chunks.push_back( TryGetChunkReference( GetRefIndexFromCenter( dx, dz ) ) );

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
    while ( temStatus < m_RequiredStatus && UpgradeSatisfied( temStatus++ ) )
    { }

    return temStatus == m_RequiredStatus;
}

bool
WorldChunk::NextStatusUpgradeSatisfied( ) const
{
    assert( m_RequiredStatus <= ChunkStatus::eFull );

    // already fulfilled
    if ( m_RequiredStatus <= m_Status ) return true;

    return UpgradeSatisfied( m_Status );
}

size_t
WorldChunk::GetObjectSize( ) const
{

    size_t result = RenderableChunk::GetObjectSize( ) + sizeof( WorldChunk ) + m_RequiredBy.capacity( ) * sizeof( m_RequiredBy[ 0 ] ) + m_StructureStarts.capacity( ) * sizeof( m_StructureStarts[ 0 ] ) + m_StructureReferences.size( ) * sizeof( m_StructureReferences.front( ) );

    for ( const auto& ss : m_StructureStarts )
        result += ss->GetObjectSize( );

    return result;
}
