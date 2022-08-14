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
        for ( int dx = -StructureReferenceStatusRange; dx <= StructureReferenceStatusRange; ++dx )
        {
            for ( int dz = -StructureReferenceStatusRange; dz <= StructureReferenceStatusRange; ++dz )
            {
                const auto  chunkCoordinate = m_Coordinate + MakeMinecraftCoordinate( dx, 0, dz );
                const auto* chunkCache      = m_World->GetChunkCache( chunkCoordinate );
                if ( chunkCache == nullptr || !chunkCache->IsChunkStatusAtLeast( ChunkStatus::eStructureStart ) )
                {
                    missingChunk.push_back( chunkCoordinate );
                    attemptFailed = true;
                }
                if ( attemptFailed ) continue;

                const auto& chunkReferenceStarts = chunkCache->GetStructureStarts( );
                m_StructureReferences.reserve( m_StructureReferences.size( ) + chunkReferenceStarts.size( ) );
                for ( const auto& chunkReferenceStart : chunkReferenceStarts )
                    m_StructureReferences.emplace_back( chunkReferenceStart );
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
Chunk::GetHeight( uint32_t index )
{
    return m_WorldHeightMap[ index ];
}

void
Chunk::SetCoordinate( const ChunkCoordinate& coordinate )
{
    m_WorldCoordinate = m_Coordinate = coordinate;
    GetMinecraftX( m_WorldCoordinate ) <<= SectionUnitLengthBinaryOffset;
    GetMinecraftZ( m_WorldCoordinate ) <<= SectionUnitLengthBinaryOffset;

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
    std::lock_guard<std::recursive_mutex> lock( m_World->GetChunkPool( ).GetChunkCacheLock( ) );
    for ( int dx = -StructureReferenceStatusRange; dx <= StructureReferenceStatusRange; ++dx )
        for ( int dz = -StructureReferenceStatusRange; dz <= StructureReferenceStatusRange; ++dz )
            if ( auto* chunkCache = m_World->GetChunkCache( m_Coordinate + MakeMinecraftCoordinate( dx, 0, dz ) );
                 chunkCache != nullptr && !chunkCache->IsChunkStatusAtLeast( ChunkStatus::eStructureStart ) ) return false;
    // chunk exist and not at target status
    // if chunk not exist AttemptRun* will add them

    return true;
}

bool
Chunk::CanRunNoise( ) const
{
    return true;
}

bool
Chunk::CanRunFeature( ) const
{
    return true;
}
