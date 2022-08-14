//
// Created by loys on 5/23/22.
//

#include "Chunk.hpp"
#include <Minecraft/World/Generation/Structure/StructureTree.hpp>

#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>

#include <cmath>   // namespace

Chunk::~Chunk( )
{
    DeleteChunk( );
}

void
Chunk::RegenerateChunk( ChunkStatus status )
{
    DeleteChunk( );
    UpgradeChunk( status );
}

void
Chunk::FillTerrain( const MinecraftNoise& generator )
{
    auto xCoordinate = GetMinecraftX( m_WorldCoordinate );
    auto zCoordinate = GetMinecraftZ( m_WorldCoordinate );

    const auto& noiseOffset = MinecraftServer::GetInstance( ).GetWorld( ).GetTerrainNoiseOffset( );

    for ( auto& height : m_WorldHeightMap )
    {
        height = std::make_unique<int32_t[]>( SectionSurfaceSize );
        for ( int i = 0; i < SectionSurfaceSize; ++i )
            height[ i ] = -1;
    }

    m_Blocks = std::make_unique<Block[]>( ChunkVolume );

    auto     blocksPtr          = m_Blocks.get( );
    uint32_t horizontalMapIndex = 0;
    for ( int i = 0; i < ChunkMaxHeight; ++i )
    {
        horizontalMapIndex = 0;
        for ( int k = 0; k < SectionUnitLength; ++k )
            for ( int j = 0; j < SectionUnitLength; ++j )
            {
                auto noiseValue = generator.GetNoiseInt( xCoordinate + j, i, zCoordinate + k );
                noiseValue += noiseOffset[ i ];

                blocksPtr[ horizontalMapIndex ] = noiseValue > 0 ? BlockID::Air : BlockID::Stone;
                if ( !blocksPtr[ horizontalMapIndex ].Transparent( ) ) m_WorldHeightMap[ eNoiseHeight ][ horizontalMapIndex ] = i;

                ++horizontalMapIndex;
            }

        blocksPtr += SectionSurfaceSize;
    }

    // surface block
    blocksPtr = m_Blocks.get( );
    for ( int i = 0; i < ChunkMaxHeight; ++i )
    {
        horizontalMapIndex = 0;
        for ( int k = 0; k < SectionUnitLength; ++k )
            for ( int j = 0; j < SectionUnitLength; ++j )
            {
                if ( i > m_WorldHeightMap[ eNoiseHeight ][ horizontalMapIndex ] - 3 )
                {
                    if ( i == m_WorldHeightMap[ eNoiseHeight ][ horizontalMapIndex ] )
                    {
                        blocksPtr[ horizontalMapIndex ] = BlockID::Grass;
                    } else if ( i <= m_WorldHeightMap[ horizontalMapIndex ] )
                    {
                        blocksPtr[ horizontalMapIndex ] = BlockID::Dart;
                    }
                }

                ++horizontalMapIndex;
            }

        blocksPtr += SectionSurfaceSize;
    }
}

void
Chunk::FillBedRock( const MinecraftNoise& generator )
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
                m_Blocks[ horizontalMapIndex + SectionSurfaceSize * i ] = BlockID ::BedRock;

            ++horizontalMapIndex;
        }
}

const Block*
Chunk::CheckBlock( const BlockCoordinate& blockCoordinate ) const
{
    if ( GetMinecraftY( blockCoordinate ) >= ChunkMaxHeight || GetMinecraftY( blockCoordinate ) < 0 ) return nullptr;
    const auto& blockIndex = ScaleToSecond<1, SectionSurfaceSize>( GetMinecraftY( blockCoordinate ) ) + ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( blockCoordinate ) ) + GetMinecraftX( blockCoordinate );
    assert( blockIndex >= 0 && blockIndex < ChunkVolume );
    return &m_Blocks[ blockIndex ];
}

Block*
Chunk::GetBlock( const BlockCoordinate& blockCoordinate )
{
    if ( GetMinecraftY( blockCoordinate ) >= ChunkMaxHeight || GetMinecraftY( blockCoordinate ) < 0 ) return nullptr;
    const auto& blockIndex = ScaleToSecond<1, SectionSurfaceSize>( GetMinecraftY( blockCoordinate ) ) + ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( blockCoordinate ) ) + GetMinecraftX( blockCoordinate );
    assert( blockIndex >= 0 && blockIndex < ChunkVolume );
    return &m_Blocks[ blockIndex ];
}

bool
Chunk::SetBlock( const BlockCoordinate& blockCoordinate, const Block& block )
{
    const auto& blockIndex = ScaleToSecond<1, SectionSurfaceSize>( GetMinecraftY( blockCoordinate ) ) + ScaleToSecond<1, SectionUnitLength>( GetMinecraftZ( blockCoordinate ) ) + GetMinecraftX( blockCoordinate );
    return SetBlock( blockIndex, block );
}

bool
Chunk::SetBlock( const uint32_t& blockIndex, const Block& block )
{
    assert( blockIndex >= 0 && blockIndex < ChunkVolume );

    // Logger::getInstance( ).LogLine( Logger::LogType::eVerbose, "Setting block at chunk:", m_Coordinate, "index:", blockIndex, "from:", toString( (BlockID) m_Blocks[ blockIndex ] ), "to:", toString( (BlockID) block ) );

    if ( m_Blocks[ blockIndex ] == block ) return false;

    // update height map
    if ( !block.Transparent( ) )
    {
        int heightMapLayerStart = m_Status >= eFeature ? eFullHeight : ( m_Status >= eNoise ? eFeatureHeight : eNoiseHeight );
        for ( ; heightMapLayerStart <= eFullHeight; ++heightMapLayerStart )
        {
            auto&      originalHeight = m_WorldHeightMap[ heightMapLayerStart ][ blockIndex & GetConstantBinaryMask<SectionSurfaceSize>( ) ];
            const auto height         = blockIndex >> SectionSurfaceSizeBinaryOffset;
            if ( originalHeight < height ) originalHeight = height;
        }
    }

    m_Blocks[ blockIndex ] = block;
    return true;
}

void
Chunk::UpgradeChunk( ChunkStatus targetStatus )
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
            break;
        case eNoise:
            if ( !AttemptRunFeature( ) ) return;
            break;
        case eFeature: break;
        }
    }
}

bool
Chunk::AttemptRunStructureStart( )
{
    StructureTree::TryGenerate( *this, m_StructureStarts );
    return true;
}

bool
Chunk::AttemptRunStructureReference( )
{
    std::vector<ChunkCoordinate> missingChunk;
    bool                         attemptFailed = false;

    {
        std::lock_guard<std::recursive_mutex> lock( m_World->GetChunkPool( ).GetChunkCacheLock( ) );
        for ( int index = 0, dx = -StructureReferenceStatusRange; dx <= StructureReferenceStatusRange; ++dx )
        {
            for ( int dz = -StructureReferenceStatusRange; dz <= StructureReferenceStatusRange; ++dz, ++index )
            {
                const auto             worldCoordinate = m_Coordinate + MakeMinecraftCoordinate( dx, 0, dz );
                const auto             weakChunkCache  = GetChunkReference( index, worldCoordinate );
                std::shared_ptr<Chunk> chunkCache;
                //                if ( weakChunkCache.expired( ) || ( chunkCache = weakChunkCache.lock( ) ) == nullptr || !chunkCache->IsChunkStatusAtLeast( ChunkStatus::eStructureStart ) )
                if ( weakChunkCache.expired( ) || !( chunkCache = weakChunkCache.lock( ) )->IsChunkStatusAtLeast( ChunkStatus::eStructureStart ) )
                {
                    missingChunk.push_back( worldCoordinate );
                    attemptFailed = true;
                }
                if ( attemptFailed ) continue;

                const auto& chunkReferenceStarts = chunkCache->GetStructureStarts( );
                m_StructureReferences.reserve( m_StructureReferences.size( ) + chunkReferenceStarts.size( ) );
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

    if ( attemptFailed )
    {
        for ( const auto& chunk : missingChunk )
            m_World->IntroduceChunk( chunk, ChunkStatus::eStructureStart );

        m_StructureReferences.clear( );
    }

    return !attemptFailed;
}

bool
Chunk::AttemptRunNoise( )
{
    FillTerrain( *MinecraftServer::GetInstance( ).GetWorld( ).GetTerrainNoise( ) );
    FillBedRock( *MinecraftServer::GetInstance( ).GetWorld( ).GetBedRockNoise( ) );

    return true;
}

bool
Chunk::AttemptRunFeature( )
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

CoordinateType
Chunk::GetHeight( uint32_t index, HeightMapStatus status )
{
    return m_WorldHeightMap[ status ][ index ];
}

void
Chunk::SetCoordinate( const ChunkCoordinate& coordinate )
{
    m_WorldCoordinate = m_Coordinate = coordinate;
    GetMinecraftX( m_WorldCoordinate ) <<= SectionUnitLengthBinaryOffset;
    GetMinecraftZ( m_WorldCoordinate ) <<= SectionUnitLengthBinaryOffset;

    minX = GetMinecraftX( m_WorldCoordinate );
    maxX = GetMinecraftX( m_WorldCoordinate ) + SectionUnitLength;
    minY = GetMinecraftZ( m_WorldCoordinate );
    maxY = GetMinecraftZ( m_WorldCoordinate ) + SectionUnitLength;

    m_ChunkNoise = GetChunkNoise( *MinecraftServer::GetInstance( ).GetWorld( ).GetTerrainNoise( ) );
}

bool
Chunk::CanRunStructureStart( ) const
{
    return true;
}

bool
Chunk::CanRunStructureReference( ) const
{
    return IsStatusAtLeastInRange( ChunkStatus::eStructureStart, StructureReferenceStatusRange );
}

bool
Chunk::CanRunNoise( ) const
{
    return true;
}

bool
Chunk::CanRunFeature( ) const
{
    return IsStatusAtLeastInRange( ChunkStatus::eNoise, 2 );
}

bool
Chunk::SetBlockAtWorldCoordinate( const BlockCoordinate& blockCoordinate, const Block& block )
{
    if ( !IsPointInside( blockCoordinate ) ) return false;

    return SetBlock( blockCoordinate - m_WorldCoordinate, block );
}

std::weak_ptr<Chunk>&
Chunk::GetChunkReference( uint32_t index, const ChunkCoordinate& worldCoordinate )
{
    if ( auto& chunkPtr = m_ChunkReferencesSaves[ index ]; !chunkPtr.expired( ) )
    {
        return chunkPtr;
    }

    return m_ChunkReferencesSaves[ index ] = m_World->GetChunkCache( worldCoordinate );
}

std::weak_ptr<Chunk>
Chunk::GetChunkReferenceConst( uint32_t index, const ChunkCoordinate& worldCoordinate ) const
{
    if ( auto& chunkPtr = m_ChunkReferencesSaves[ index ]; !chunkPtr.expired( ) )
    {
        return chunkPtr;
    }

    return { };
}

bool
Chunk::UpgradeStatusAtLeastInRange( ChunkStatus targetStatus, int range )
{
    assert( range <= ChunkReferenceRange );
    int unitOffset = StructureReferenceStatusRange - range;

    std::vector<ChunkCoordinate> missingChunk;

    {
        std::lock_guard<std::recursive_mutex> lock( m_World->GetChunkPool( ).GetChunkCacheLock( ) );
        for ( int dz = -range; dz <= range; ++dz )
            for ( int dx = -range; dx <= range; ++dx )
            {
                const auto chunkCoordinate = m_Coordinate + MakeMinecraftCoordinate( dx, 0, dz );
                if ( auto chunkCache = GetChunkReferenceConst( ( ChunkReferenceRange * ( unitOffset + dz + range ) ) + unitOffset + ( dx + range ), m_Coordinate + MakeMinecraftCoordinate( dx, 0, dz ) );
                     chunkCache.expired( ) || !chunkCache.lock( )->IsChunkStatusAtLeast( targetStatus ) )
                    missingChunk.push_back( chunkCoordinate );
            }
        // chunk exist and not at target status
        // if chunk not exist AttemptRun* will add them
    }

    for ( const auto& chunkCoordinate : missingChunk )
        m_World->IntroduceChunk( chunkCoordinate, ChunkStatus::eStructureStart );

    return !missingChunk.empty( );
}

bool
Chunk::IsStatusAtLeastInRange( ChunkStatus targetStatus, int range ) const
{
    assert( range <= ChunkReferenceRange );
    int unitOffset = StructureReferenceStatusRange - range;

    std::lock_guard<std::recursive_mutex> lock( m_World->GetChunkPool( ).GetChunkCacheLock( ) );
    for ( int dz = -range; dz <= range; ++dz )
        for ( int dx = -range; dx <= range; ++dx )
            if ( auto chunkCache = GetChunkReferenceConst( ( ChunkReferenceRange * ( unitOffset + dz + range ) ) + unitOffset + ( dx + range ), m_Coordinate + MakeMinecraftCoordinate( dx, 0, dz ) );
                 !chunkCache.expired( ) && !chunkCache.lock( )->IsChunkStatusAtLeast( targetStatus ) ) return false;
    // chunk exist and not at target status
    // if chunk not exist AttemptRun* will add them

    return true;
}

std::vector<std::weak_ptr<Chunk>>
Chunk::GetChunkRefInRange( int range )
{
    assert( range <= ChunkReferenceRange );
    int unitOffset = StructureReferenceStatusRange - range;

    std::vector<std::weak_ptr<Chunk>> chunks;

    {
        std::lock_guard<std::recursive_mutex> lock( m_World->GetChunkPool( ).GetChunkCacheLock( ) );
        for ( int dz = -range; dz <= range; ++dz )
            for ( int dx = -range; dx <= range; ++dx )
            {
                const auto chunkCoordinate    = m_Coordinate + MakeMinecraftCoordinate( dx, 0, dz );
                const auto chunkRelativeIndex = ( ChunkReferenceRange * ( unitOffset + dz + range ) ) + unitOffset + ( dx + range );
                chunks.push_back( GetChunkReferenceConst( chunkRelativeIndex, chunkCoordinate ) );
            }
    }

    return chunks;
}
