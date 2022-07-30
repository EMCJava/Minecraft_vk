//
// Created by loys on 5/22/22.
//

#include "MinecraftWorld.hpp"

#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>

#include <Include/GlobalConfig.hpp>
#include <random>

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

    m_ChunkPool         = std::make_unique<ChunkPool>( GlobalConfig::getMinecraftConfigData( )[ "chunk" ][ "loading_thread" ].get<int>( ) );
    m_ChunkLoadingRange = GlobalConfig::getMinecraftConfigData( )[ "chunk" ][ "chunk_loading_range" ].get<CoordinateType>( );
    m_ChunkPool->SetValidRange( m_ChunkLoadingRange );
    m_ChunkPool->StartThread( );
}

void
MinecraftWorld::IntroduceChunkInRange( const ChunkCoordinate& centre, int32_t radius )
{
    m_ChunkPool->SetCentre( centre );
    for ( int i = -radius; i <= radius; ++i )
        for ( int j = -radius; j <= radius; ++j )
            m_ChunkPool->AddCoordinate( centre + MakeMinecraftCoordinate( i, 0, j ) );
}

void
MinecraftWorld::Tick( float deltaTime )
{
    Tickable::Tick( deltaTime );

    m_TimeSinceChunkLoad += deltaTime;
    if ( m_TimeSinceChunkLoad > 0.2f )
    {
        auto chunkCoordinate      = MinecraftServer::GetInstance( ).GetPlayer( 0 ).GetChunkCoordinate( );
        get<2>( chunkCoordinate ) = 0;
        IntroduceChunkInRange( chunkCoordinate, m_ChunkLoadingRange );

        if ( m_ChunkPool->IsChunkErased( ) )   // reload cache
        {
            std::lock_guard<std::mutex> lock( m_ChunkPool->GetChunkCacheLock( ) );
            const auto&                 firstCache = *m_ChunkPool->GetChunkIterBegin( );

            for ( uint32_t i = 0; i < ChunkAccessCacheSize; ++i )
                if ( m_ChunkPool->GetChunkCache( m_ChunkAccessCache[ i ].coordinates ) == nullptr )
                {
                    m_ChunkAccessCache[ i ].coordinates = firstCache.first;
                    m_ChunkAccessCache[ i ].chunkCache  = firstCache.second.get( );
                }
        }

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

    if ( auto* chunkCache = GetCompleteChunkCache( MakeMinecraftCoordinate( ScaleToSecond<SectionUnitLength, 1>( GetMinecraftX( blockCoordinate ) ),
                                                                            0,
                                                                            ScaleToSecond<SectionUnitLength, 1>( GetMinecraftZ( blockCoordinate ) ) ) );
         chunkCache != nullptr )
    {
        return chunkCache->GetBlock( MakeMinecraftCoordinate( GetMinecraftX( blockCoordinate ) & ( SectionUnitLength - 1 ),
                                                              GetMinecraftY( blockCoordinate ),
                                                              GetMinecraftZ( blockCoordinate ) & ( SectionUnitLength - 1 ) ) );
    }

    return nullptr;
}
ChunkCache*
MinecraftWorld::GetCompleteChunkCache( const ChunkCoordinate& chunkCoordinate )
{
    if ( auto* chunkCache = GetChunkCache( chunkCoordinate ); chunkCache != nullptr && chunkCache->initialized )
    {
        return chunkCache;
    }

    return nullptr;
}

ChunkCache*
MinecraftWorld::GetChunkCache( const ChunkCoordinate& chunkCoordinate )
{
    for ( uint32_t i = ChunkAccessCacheIndex; i < ChunkAccessCacheSize; ++i )
        if ( m_ChunkAccessCache[ i ].coordinates == chunkCoordinate ) return m_ChunkAccessCache[ i ].chunkCache;
    for ( uint32_t i = 0; i < ChunkAccessCacheIndex; ++i )
        if ( m_ChunkAccessCache[ i ].coordinates == chunkCoordinate ) return m_ChunkAccessCache[ i ].chunkCache;

    ChunkAccessCacheIndex                                         = ( ChunkAccessCacheIndex + 1 ) % ChunkAccessCacheSize;
    m_ChunkAccessCache[ ChunkAccessCacheIndex ].coordinates       = chunkCoordinate;
    return m_ChunkAccessCache[ ChunkAccessCacheIndex ].chunkCache = m_ChunkPool->GetChunkCache( chunkCoordinate );
}

void
MinecraftWorld::ResetChunkCache( )
{
    m_ChunkPool->AddCoordinate( { 0, 0, 0 } );
    for ( uint32_t i = 0; i < ChunkAccessCacheSize; ++i )
    {
        m_ChunkAccessCache[ i ].coordinates = { 0, 0, 0 };
        m_ChunkAccessCache[ i ].chunkCache  = m_ChunkPool->GetChunkCache( { 0, 0, 0 } );
    }
}

bool
MinecraftWorld::SetBlock( const BlockCoordinate& blockCoordinate, const Block& block )
{
    if ( GetMinecraftY( blockCoordinate ) < 0 ) return false;

    if ( auto* chunkCache = GetChunkCache( MakeMinecraftCoordinate( ScaleToSecond<SectionUnitLength, 1>( GetMinecraftX( blockCoordinate ) ),
                                                                    0,
                                                                    ScaleToSecond<SectionUnitLength, 1>( GetMinecraftZ( blockCoordinate ) ) ) );
         chunkCache != nullptr && chunkCache->initialized )
    {
        if ( chunkCache->SetBlock( MakeMinecraftCoordinate( GetMinecraftX( blockCoordinate ) & ( SectionUnitLength - 1 ),
                                                            GetMinecraftY( blockCoordinate ),
                                                            GetMinecraftZ( blockCoordinate ) & ( SectionUnitLength - 1 ) ),
                                   block ) )
        {
            chunkCache->GenerateRenderBuffer( );
            return true;
        }
    }

    return false;
}
