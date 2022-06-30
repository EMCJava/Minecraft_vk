//
// Created by loys on 5/24/22.
//

#ifndef MINECRAFT_VK_CHUNKPOOL_HPP
#define MINECRAFT_VK_CHUNKPOOL_HPP

#include <mutex>

#include "ChunkCache.hpp"

#include <Utility/Logger.hpp>
#include <Utility/Thread/ThreadPool.hpp>

class ChunkPool : public ThreadPool<ChunkCache>
{
private:
    CoordinateType                m_RemoveJobAfterRange = 0;
    BlockCoordinate               m_PrioritizeCoordinate;
    std::unique_ptr<std::jthread> m_UpdateThread;

    std::mutex                                       m_ChunkCacheLock;
    std::unordered_map<BlockCoordinate, ChunkCache*> m_ChunkCache;

    static void LoadChunk( ChunkCache* cache )
    {
        cache->initializing = true;
        cache->ResetLoad( );
    }

    void UpdateThread( const std::stop_token& st )
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
                for ( auto& cache : finished )
                {
                    cache->initializing = false;
                    cache->initialized  = true;
                }
            } else
            {
                std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
            }
        }

        while ( !m_RunningThreads.empty( ) )
        {
            Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Waiting", m_RunningThreads.size( ), "thread to be finished" );
            CleanRunningThread( );
            std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
        }
    }

public:
    explicit ChunkPool( uint32_t maxThread )
        : ThreadPool<ChunkCache>( maxThread )
    { }

    ~ChunkPool( )
    {
        m_UpdateThread.reset( );
        std::for_each( m_ChunkCache.begin( ), m_ChunkCache.end( ), []( const auto& pair ) { delete pair.second; } );
    }

    void SetCentre( const BlockCoordinate& centre )
    {
        m_PrioritizeCoordinate = centre;
    }

    void SetValidRange( CoordinateType range )
    {
        m_RemoveJobAfterRange = range;
    }

    void StartThread( )
    {
        if ( m_UpdateThread ) return;
        m_UpdateThread = std::make_unique<std::jthread>( std::bind_front( &ChunkPool::UpdateThread, this ) );
    }

    void AddCoordinate( const BlockCoordinate& coordinate )
    {
        if ( m_ChunkCache.contains( coordinate ) ) return;

        auto newChunk = new ChunkCache;
        newChunk->chunk.SetCoordinate( coordinate );

        AddJobContext( newChunk );
        m_ChunkCache.insert( { coordinate, newChunk } );
    }

    bool IsChunkLoading( const BlockCoordinate& coordinate )
    {
        if ( auto find_it = m_ChunkCache.find( coordinate ); find_it != m_ChunkCache.end( ) )
        {
            return find_it->second->initializing;
        }
        return false;
    }

    bool IsChunkLoaded( const BlockCoordinate& coordinate )
    {
        if ( auto find_it = m_ChunkCache.find( coordinate ); find_it != m_ChunkCache.end( ) )
        {
            return find_it->second->initialized;
        }
        return false;
    }

    size_t GetPendingCount( )
    {
        return m_PendingThreads.size( );
    }

    size_t GetLoadingCount( )
    {
        return m_RunningThreads.size( );
    }

    size_t GetTotalChunk( )
    {
        return m_ChunkCache.size( );
    }

    auto& GetChunkCacheLock( )
    {
        return m_ChunkCacheLock;
    }

    auto GetChunkIterBegin( )
    {
        return m_ChunkCache.begin( );
    }

    auto GetChunkIterEnd( )
    {
        return m_ChunkCache.end( );
    }
};


#endif   // MINECRAFT_VK_CHUNKPOOL_HPP
