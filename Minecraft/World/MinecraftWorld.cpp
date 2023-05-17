//
// Created by loys on 5/22/22.
//

#include "MinecraftWorld.hpp"

#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>
#include <Minecraft/World/Chunk/ChunkPool.hpp>

#include <Include/GlobalConfig.hpp>
#include <random>

MinecraftWorld::~MinecraftWorld( ) = default;
MinecraftWorld::MinecraftWorld( )
{
    Logger ::getInstance( ).LogLine( "Using \"random\" seed" );
    std::random_device                      rd;
    std::mt19937_64                         gen( rd( ) );
    std::uniform_int_distribution<uint64_t> dis;

    m_WorldTerrainNoise = std::make_unique<MinecraftNoise>( std::pair<uint64_t, uint64_t> { dis( gen ), dis( gen ) } );
    m_WorldTerrainNoise->SetNoiseType( Noise::FastNoiseLite::NoiseType_Perlin );
    m_WorldTerrainNoise->SetFractalType( Noise::FastNoiseLite::FractalType_FBm );
    m_WorldTerrainNoise->SetFractalLacunarity( 2.5f );
    m_WorldTerrainNoise->SetFractalGain( 0.32f );

    m_WorldTerrainNoise->SetFractalOctaves( 8 );

    m_BedRockNoise = std::make_unique<MinecraftNoise>( std::pair<uint64_t, uint64_t> { dis( gen ), dis( gen ) } );
    m_BedRockNoise->SetFrequency( 16 );
    m_BedRockNoise->SetFractalOctaves( 1 );

    m_TerrainNoiseOffsetPerLevel = std::make_unique<float[]>( ChunkMaxHeight );
    for ( int i = 0; i < ChunkMaxHeight; ++i )
        m_TerrainNoiseOffsetPerLevel[ i ] = 0;

    m_ChunkPool         = std::make_unique<ChunkPool>( this, GlobalConfig::getMinecraftConfigData( )[ "chunk" ][ "loading_thread" ].get<int>( ) );
    m_ChunkLoadingRange = GlobalConfig::getMinecraftConfigData( )[ "chunk" ][ "chunk_loading_range" ].get<CoordinateType>( );

    std::array<int32_t, ChunkStatusSize> statusValidRange;
    statusValidRange[ eEmpty ]   = -1;
    statusValidRange[ eFeature ] = m_ChunkLoadingRange;

    statusValidRange[ eFeature - 1 ]            = statusValidRange[ eFeature ] + FeatureStatusRange;
    statusValidRange[ eNoise - 1 ]              = statusValidRange[ eNoise ];
    statusValidRange[ eStructureReference - 1 ] = statusValidRange[ eStructureReference ] + StructureReferenceStatusRange;
    statusValidRange[ eStructureStart - 1 ]     = statusValidRange[ eStructureStart ];

    m_ChunkPool->SetStatusValidRange( statusValidRange );
    m_ChunkPool->StartThread( );
}

void
MinecraftWorld::IntroduceChunkInRange( const ChunkCoordinate& centre, int32_t radius )
{
    m_ChunkPool->SetCentre( centre );
    for ( int i = -radius; i <= radius; ++i )
        for ( int j = -radius; j <= radius; ++j )
            IntroduceChunk( centre + MakeMinecraftChunkCoordinate( i, j ), ChunkStatus::eFull );
}

void
MinecraftWorld::Tick( float deltaTime )
{
    Tickable::Tick( deltaTime );

    m_TimeSinceChunkLoad += deltaTime;
    if ( m_TimeSinceChunkLoad > 0.2f )
    {
        auto chunkCoordinate = MinecraftServer::GetInstance( ).GetPlayer( 0 ).GetChunkCoordinate( );
        IntroduceChunkInRange( chunkCoordinate, m_ChunkLoadingRange );

        m_TimeSinceChunkLoad = 0;
    }
}

void
MinecraftWorld::StartChunkGeneration( )
{
    m_ChunkPool->StartThread( );
}

void
MinecraftWorld::StopChunkGeneration( )
{
    m_ChunkPool->StopThread( );
}

void
MinecraftWorld::CleanChunk( )
{
    m_ChunkPool->Clean( );
}

Block*
MinecraftWorld::GetBlock( const BlockCoordinate& blockCoordinate )
{
    if ( GetMinecraftY( blockCoordinate ) < 0 ) return nullptr;

    if ( auto chunkCache = GetCompleteChunkCache( BlockToChunkWorldCoordinate( blockCoordinate ) );
         chunkCache != nullptr )
    {
        return chunkCache->GetBlock( BlockToChunkRelativeCoordinate( blockCoordinate ) );
    }

    return nullptr;
}

std::shared_ptr<ChunkTy>
MinecraftWorld::GetCompleteChunkCache( const ChunkCoordinate& chunkCoordinate )
{
    if ( auto chunkCache = GetChunkCacheUnsafe( chunkCoordinate ); chunkCache != nullptr )
    {
        if ( chunkCache->initialized && chunkCache->IsChunkStatusAtLeast( ChunkStatus::eFull ) )
        {
            return chunkCache;
        }
    }

    return { };
}

std::shared_ptr<ChunkTy>
MinecraftWorld::GetChunkCacheSafe( const ChunkCoordinate& chunkCoordinate )
{
    return m_ChunkPool->GetChunkCacheSafe( chunkCoordinate );
}

std::shared_ptr<ChunkTy>
MinecraftWorld::GetChunkCacheUnsafe( const ChunkCoordinate& chunkCoordinate )
{
    return m_ChunkPool->GetChunkCacheUnsafe( chunkCoordinate );
}

bool
MinecraftWorld::SetBlock( const BlockCoordinate& blockCoordinate, const Block& block )
{
    if ( GetMinecraftY( blockCoordinate ) < 0 ) return false;

    if ( auto chunkCache = GetChunkCacheUnsafe( MakeMinecraftChunkCoordinate( ScaleToSecond<SectionUnitLength, 1>( GetMinecraftX( blockCoordinate ) ),
                                                                              ScaleToSecond<SectionUnitLength, 1>( GetMinecraftZ( blockCoordinate ) ) ) );
         chunkCache != nullptr )
    {
        if ( chunkCache->initialized && chunkCache->SetBlock( BlockToChunkRelativeCoordinate( blockCoordinate ), block ) )
        {
            chunkCache->GenerateRenderBuffer( );
            return true;
        }
    }

    return false;
}

void
MinecraftWorld::IntroduceChunk( const ChunkCoordinate& position, ChunkStatus minimumStatus )
{
    m_ChunkPool->AddCoordinateSafe( position, minimumStatus );
}
