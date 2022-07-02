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
    while ( !st.stop_requested( ) )
    {
        if ( m_RemoveJobAfterRange > 0 )
        {
            std::lock_guard<std::mutex> guard( m_PendingThreadsMutex );

            const auto lastIt = std::partition( m_PendingThreads.begin( ), m_PendingThreads.end( ),
                                                [ range = m_RemoveJobAfterRange << 1, centre = m_PrioritizeCoordinate ]( const auto& cache ) { return cache->chunk.ManhattanDistance( centre ) < range; } );

            const auto amountToRemove = std::distance( lastIt, m_PendingThreads.end( ) );

            // elements outside the range
            if ( amountToRemove > 0 )
            {
                std::lock_guard cacheModifyLocker( m_ChunkCacheLock );
                for ( auto it = lastIt; it != m_PendingThreads.end( ); ++it )
                    m_ChunkCache.erase( ( *it )->chunk.GetCoordinate( ) );
                m_PendingThreads.erase( lastIt, m_PendingThreads.end( ) );
            }
        }

        auto finished = UpdateSorted( &ChunkPool::LoadChunk,
                                      [ centre = m_PrioritizeCoordinate ]( const ChunkCache* a, const ChunkCache* b ) {
                                          return a->chunk.ManhattanDistance( centre ) < b->chunk.ManhattanDistance( centre );
                                      } );
        if ( !finished.empty( ) )
        {
            Timer timer;

            for ( auto& cache : finished )
            {

                cache->initializing = false;
                cache->initialized  = true;

                for ( int i = 0; i < DirHorizontalSize; ++i )
                {
                    auto chunkPtr = MinecraftServer::GetInstance( ).GetWorld( ).GetChunkPool( ).GetChunk( cache->chunk.GetCoordinate( ) + NearChunkDirection[ i ] );
                    if ( chunkPtr != nullptr && chunkPtr->initialized )
                    {
                        // this is fast, I guess?
                        if ( cache->chunk.SyncChunkFromDirection( &chunkPtr->chunk, static_cast<Direction>( i ) ) ) cache->ResetModel( );
                        if ( chunkPtr->chunk.SyncChunkFromDirection( &cache->chunk, static_cast<Direction>( i ^ 0b1 ) ) ) chunkPtr->ResetModel( );
                    }
                }
            }
        } else
        {
            // std::this_thread::yield();
            std::this_thread::sleep_for( std::chrono::milliseconds( ChunkThreadDelayPeriod ) );
        }
    }

    while ( !m_RunningThreads.empty( ) )
    {
        Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Waiting", m_RunningThreads.size( ), "thread to be finished" );
        CleanRunningThread( );
        std::this_thread::sleep_for( std::chrono::milliseconds( ChunkThreadDelayPeriod ) );
    }
}
