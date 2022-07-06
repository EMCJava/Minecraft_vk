//
// Created by loys on 5/24/22.
//

#ifndef MINECRAFT_VK_CHUNKPOOL_HPP
#define MINECRAFT_VK_CHUNKPOOL_HPP

#include <mutex>

#include "ChunkCache.hpp"
#include "ChunkRenderBuffers.hpp"

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

    std::mutex       m_RenderBufferLock;
    ChunkSolidBuffer m_ChunkRenderBuffers;

    static void LoadChunk( ChunkCache* cache )
    {
        cache->initializing = true;
        cache->ResetLoad( );
    }

    void UpdateThread( const std::stop_token& st );

public:
    explicit ChunkPool( uint32_t maxThread )
        : ThreadPool<ChunkCache>( maxThread )
    { }

    ~ChunkPool( )
    {
        m_UpdateThread.reset( );
        std::for_each( m_ChunkCache.begin( ), m_ChunkCache.end( ), []( const auto& pair ) { delete pair.second; } );
    }

    inline void SetCentre( const BlockCoordinate& centre )
    {
        m_PrioritizeCoordinate = centre;
    }

    inline void SetValidRange( CoordinateType range )
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

    inline auto& GetRenderBufferLock( )
    {
        return m_RenderBufferLock;
    }

    inline auto& GetRenderBuffer( )
    {
        return m_ChunkRenderBuffers;
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

    ChunkCache* GetChunk( const ChunkCoordinate& coordinate ) const
    {
        if ( auto it = m_ChunkCache.find( coordinate ); it != m_ChunkCache.end( ) )
        {
            return it->second;
        }

        return nullptr;
    }
};


#endif   // MINECRAFT_VK_CHUNKPOOL_HPP
