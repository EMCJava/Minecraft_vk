//
// Created by loys on 5/24/22.
//

#include "ChunkPool.hpp"
#include <Utility/Timer.hpp>

#include <Minecraft/Internet/MinecraftServer/MinecraftServer.hpp>

namespace
{

template <uint32_t dir>
constexpr ChunkCoordinate
getNearChunkDirection( );

template <>
constexpr ChunkCoordinate
getNearChunkDirection<DirFront>( )
{
    return MakeMinecraftCoordinate( 1, 0, 0 );
}

template <>
constexpr ChunkCoordinate
getNearChunkDirection<DirBack>( )
{
    return MakeMinecraftCoordinate( -1, 0, 0 );
}

template <>
constexpr ChunkCoordinate
getNearChunkDirection<DirRight>( )
{
    return MakeMinecraftCoordinate( 0, 0, 1 );
}

template <>
constexpr ChunkCoordinate
getNearChunkDirection<DirLeft>( )
{
    return MakeMinecraftCoordinate( 0, 0, -1 );
}

constexpr std::array<ChunkCoordinate, DirHorizontalSize> NearChunkDirection { getNearChunkDirection<0>( ), getNearChunkDirection<1>( ), getNearChunkDirection<2>( ), getNearChunkDirection<3>( ) };
}   // namespace


void
ChunkPool::UpdateThread( const std::stop_token& st )
{
    std::vector<ChunkCache*> finished( m_maxThread );

    while ( !st.stop_requested( ) )
    {
        if ( m_RemoveJobAfterRange > 0 )
        {
            std::lock_guard<std::recursive_mutex> cacheModifyLocker( m_ChunkCacheLock );
            std::lock_guard<std::recursive_mutex> guard( m_PendingThreadsMutex );
            const auto                            lastIt = std::partition( m_PendingThreads.begin( ), m_PendingThreads.end( ),
                                                                           [ range = m_RemoveJobAfterRange << 1, centre = m_PrioritizeCoordinate ]( const auto& cache ) { return cache->ManhattanDistance( centre ) <= range; } );

            const auto amountToRemove = std::distance( lastIt, m_PendingThreads.end( ) );

            // elements outside the range
            if ( amountToRemove > 0 )
                m_PendingThreads.erase( lastIt, m_PendingThreads.end( ) );

            //            std::for_each( m_ChunkCache.begin( ), m_ChunkCache.end( ), [ range = m_RemoveJobAfterRange << 1, centre = m_PrioritizeCoordinate ]( const auto& cache ) {
            //                if ( cache.second->ManhattanDistance( centre ) > range && ( !cache.second->initializing || cache.second->initialized ) )
            //                {
            //                    Logger ::getInstance( ).LogLine( Logger::LogType::eInfo, "About to erase chunk outside range", cache.first, cache.second.get( ) );
            //                }
            //            } );

            if ( std::erase_if( m_ChunkCache, [ range = m_RemoveJobAfterRange << 1, centre = m_PrioritizeCoordinate ]( const auto& cache ) { return cache.second->ManhattanDistance( centre ) > range && ( !cache.second->initializing || cache.second->initialized ); } ) > 0 )
            {
                m_ChunkErased.test_and_set( );
            }
        }

        UpdateSorted( [ this ]( ChunkCache* cache ) { ChunkPool::LoadChunk( this, cache ); },
                      [ centre = m_PrioritizeCoordinate ]( const ChunkCache* a, const ChunkCache* b ) {
                          const auto aUpgradeable = a->NextStatusUpgradeSatisfied( );
                          const auto bUpgradeable = b->NextStatusUpgradeSatisfied( );
                          if ( aUpgradeable != bUpgradeable ) return aUpgradeable;

                          const auto aEmergency = a->GetEmergencyLevel( );
                          const auto bEmergency = b->GetEmergencyLevel( );
                          if ( aEmergency != bEmergency )
                              return aEmergency < bEmergency;

                          return a->ManhattanDistance( centre ) < b->ManhattanDistance( centre );
                      },
                      &finished );

        if ( finished.empty( ) )
        {
            // std::this_thread::yield();
            std::this_thread::sleep_for( std::chrono::milliseconds( ChunkThreadDelayPeriod ) );
            continue;
        }

        for ( auto& cache : finished )
        {
            cache->initializing = false;
            cache->initialized  = true;

            if ( cache->GetStatus( ) == ChunkStatus::eFull )
            {
                for ( int i = 0; i < DirHorizontalSize; ++i )
                {
                    auto chunkPtr = MinecraftServer::GetInstance( ).GetWorld( ).GetChunkPool( ).GetChunkCache( cache->GetCoordinate( ) + NearChunkDirection[ i ] );
                    if ( chunkPtr != nullptr && chunkPtr->initialized && chunkPtr->GetStatus( ) == ChunkStatus::eFull )
                    {
                        // this is fast, I guess? (nope)

                        // this is ok, I guess
                        // std::lock_guard<std::mutex> lock( m_RenderBufferLock );
                        if ( cache->SyncChunkFromDirection( chunkPtr, static_cast<Direction>( i ) ) ) cache->GenerateRenderBuffer( );
                        if ( chunkPtr->SyncChunkFromDirection( cache, static_cast<Direction>( i ^ 0b1 ) ) ) chunkPtr->GenerateRenderBuffer( );
                    }
                }
            }
        }
    }

    while ( !m_RunningThreads.empty( ) )
    {
        Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Waiting", m_RunningThreads.size( ), "thread to be finished" );
        CleanRunningThread( );
        std::this_thread::sleep_for( std::chrono::milliseconds( ChunkThreadDelayPeriod ) );
    }
}