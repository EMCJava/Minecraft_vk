//
// Created by loys on 5/24/22.
//

#ifndef MINECRAFT_VK_CHUNKPOOL_HPP
#define MINECRAFT_VK_CHUNKPOOL_HPP

#include <mutex>

#include "ChunkRenderBuffers.hpp"
#include "WorldChunk.hpp"

#include <Utility/Logger.hpp>
#include <Utility/Thread/ThreadPool.hpp>

using ChunkTy = WorldChunk;
class ChunkPool : public ThreadPool<ChunkTy>
{
private:
    class MinecraftWorld* m_World;

    CoordinateType                m_RemoveJobAfterRange = 0;
    BlockCoordinate               m_PrioritizeCoordinate;
    std::unique_ptr<std::jthread> m_UpdateThread;

    std::recursive_mutex                                             m_ChunkCacheLock;
    std::unordered_map<BlockCoordinate, std::shared_ptr<ChunkTy>> m_ChunkCache;

    std::atomic_flag m_ChunkErased = ATOMIC_FLAG_INIT;

    void LoadChunk( ChunkPool* pool, ChunkTy* cache )
    {
        // Logger::getInstance( ).LogLine( "Start loading", cache );

        cache->initialized  = false;
        cache->initializing = true;
        cache->TryUpgradeChunk( );

        if ( !cache->IsAtLeastTargetStatus( ) )
        {
            // Logger::getInstance( ).LogLine( "Load not complete, re-appending chunk", cache );
            pool->AddJobContext( cache );
        }
    }

    void UpdateThread( const std::stop_token& st );

public:
    explicit ChunkPool( class MinecraftWorld* world, uint32_t maxThread )
        : m_World( world )
        , ThreadPool<ChunkTy>( maxThread )
    { }

    ~ChunkPool( )
    {
        StopThread( );
        Clean( );
    }

    inline void SetCentre( const BlockCoordinate& centre )
    {
        m_PrioritizeCoordinate = centre;
    }

    inline void SetValidRange( CoordinateType range )
    {
        m_RemoveJobAfterRange = range;
    }

    void StopThread( )
    {
        m_UpdateThread.reset( );
    }

    void StartThread( )
    {
        if ( m_UpdateThread ) return;
        m_UpdateThread = std::make_unique<std::jthread>( std::bind_front( &ChunkPool::UpdateThread, this ) );
    }

    void Clean( )
    {
        m_PendingThreads.clear( );
        m_ChunkCache.clear( );
    }

    ChunkTy* AddCoordinate( const BlockCoordinate& coordinate, ChunkStatus status = ChunkStatus::eFull )
    {
        ChunkTy* newChunk = nullptr;

        {
            std::lock_guard<std::recursive_mutex> chunkLock( m_ChunkCacheLock );
            std::lock_guard<std::recursive_mutex> threadLock( m_PendingThreadsMutex );
            if ( auto find_it = m_ChunkCache.find( coordinate ); find_it != m_ChunkCache.end( ) )
            {
                // need upgrade
                if ( find_it->second->GetTargetStatus( ) < status )
                {
                    find_it->second->SetExpectedStatus( status );

                    // still in queue, just modify the target status
                    // if ( !find_it->second->initializing && !find_it->second->initialized )
                    // {
                    // } else

                    if ( find_it->second->initialized || find_it->second->initializing )   //  previous job already started or ended, need to add job for upgrading
                    {
                        AddJobContext( find_it->second.get( ) );
                    }
                }

                return find_it->second.get( );
            }

            newChunk = new ChunkTy( m_World );
            newChunk->SetCoordinate( coordinate );
            newChunk->SetExpectedStatus( status );

            m_ChunkCache.insert( { coordinate, std::unique_ptr<ChunkTy>( newChunk ) } );
        }

        AddJobContext( newChunk );
        return newChunk;
    }

    bool IsChunkLoading( const BlockCoordinate& coordinate ) const
    {
        if ( auto find_it = m_ChunkCache.find( coordinate ); find_it != m_ChunkCache.end( ) )
        {
            return find_it->second->initializing;
        }
        return false;
    }

    bool IsChunkLoaded( const BlockCoordinate& coordinate ) const
    {
        if ( auto find_it = m_ChunkCache.find( coordinate ); find_it != m_ChunkCache.end( ) )
        {
            return find_it->second->initialized;
        }
        return false;
    }

    inline size_t GetPendingCount( ) const
    {
        return m_PendingThreads.size( );
    }

    inline size_t GetLoadingCount( ) const
    {
        return m_RunningThreads.size( );
    }

    inline size_t GetTotalChunk( ) const
    {
        return m_ChunkCache.size( );
    }

    inline auto& GetChunkCacheLock( )
    {
        return m_ChunkCacheLock;
    }

    inline auto GetChunkIterBegin( ) const
    {
        return m_ChunkCache.begin( );
    }

    inline auto GetChunkIterEnd( ) const
    {
        return m_ChunkCache.end( );
    }

    inline bool IsChunkErased( )
    {
        bool result = m_ChunkErased.test( );
        m_ChunkErased.clear( );
        return result;
    }

    [[nodiscard]] std::shared_ptr<ChunkTy> GetChunkCache( const ChunkCoordinate& coordinate )
    {
        std::lock_guard<std::recursive_mutex> chunkLock( m_ChunkCacheLock );
        if ( auto it = m_ChunkCache.find( coordinate ); it != m_ChunkCache.end( ) )
        {
            return it->second;
        }

        return { };
    }
};


#endif   // MINECRAFT_VK_CHUNKPOOL_HPP
