//
// Created by loys on 5/24/22.
//

#ifndef MINECRAFT_VK_CHUNKPOOL_HPP
#define MINECRAFT_VK_CHUNKPOOL_HPP

#include "ChunkCache.hpp"

#include <Utility/Logger.hpp>
#include <Utility/Thread/ThreadPool.hpp>

class ChunkPool : public ThreadPool<ChunkCache>
{
private:
    CoordinateType                                   m_RemoveJobAfterRange = 0;
    BlockCoordinate                                  m_PrioritizeCoordinate;
    std::unique_ptr<std::jthread>                    m_UpdateThread;
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

                m_PendingThreads.erase( std::remove_if( m_PendingThreads.begin( ), m_PendingThreads.end( ),
                                                        [ range = m_RemoveJobAfterRange, centre = m_PrioritizeCoordinate ]( const auto& cache ) { return cache->chunk.ManhattanDistance( centre ) > range; } ),
                                        m_PendingThreads.end( ) );
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

    void SetCentre( BlockCoordinate centre )
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

    void AddCoordinate( BlockCoordinate coordinate )
    {
        if ( m_ChunkCache.contains( coordinate ) ) return;

        auto newChunk = new ChunkCache;
        newChunk->chunk.SetCoordinate( coordinate );

        AddJobContext( newChunk );
        m_ChunkCache.insert( { coordinate, newChunk } );
    }

    bool IsChunkLoading( BlockCoordinate coordinate )
    {
        if ( auto find_it = m_ChunkCache.find( coordinate ); find_it != m_ChunkCache.end( ) )
        {
            return find_it->second->initializing;
        }
        return false;
    }

    bool IsChunkLoaded( BlockCoordinate coordinate )
    {
        if ( auto find_it = m_ChunkCache.find( coordinate ); find_it != m_ChunkCache.end( ) )
        {
            return find_it->second->initialized;
        }
        return false;
    }
};


#endif   // MINECRAFT_VK_CHUNKPOOL_HPP
