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

#include "ChunkPoolType.hpp"

class ChunkPool : public ThreadPool<ChunkTy>
{
private:
    class MinecraftWorld* m_World;

    std::array<int32_t, ChunkStatusSize>    m_StatusJobRemoveRange { };
    int32_t                       m_MaxRemoveJobRange = 0;
    ChunkCoordinate               m_PrioritizeCoordinate;
    std::unique_ptr<std::jthread> m_UpdateThread;

    std::recursive_mutex                                              m_ChunkCacheLock;
    std::unordered_map<ChunkCoordinateHash, std::shared_ptr<ChunkTy>> m_ChunkCache;

    std::atomic_flag m_ChunkErased = ATOMIC_FLAG_INIT;

    std::mutex                                         m_SafeAddedChunksLock;
    std::unordered_map<ChunkCoordinate, ChunkStatusTy> m_SafeAddedChunks;

    static inline void LoadChunk( ChunkTy* cache );

    void CleanUpJobs( );

    /*
     *
     * This should only be called from UpdateThread when all thread are stopped.
     *
     * */
    void RemoveChunkOutsizeRange( );
    void UpdateThread( const std::stop_token& st );

    /*
     *
     * This Function should only be called in the main thread to avoid te use of mutex
     * Namely m_ChunkCache and m_PendingThreads
     *
     * */
    ChunkTy* AddCoordinate( const ChunkCoordinate& coordinate, ChunkStatus status = ChunkStatus::eFull );
    void     FlushSafeAddedChunks( );

public:
    explicit ChunkPool( class MinecraftWorld* world, uint32_t maxThread )
        : m_World( world )
        , ThreadPool<ChunkTy>( maxThread )
    { }

    ~ChunkPool( )
    {
        LOGL_SYS( "Destroying ChunkPool" )

        StopThread( );
        Clean( );
    }

    static const std::array<ChunkCoordinate, EightWayDirectionSize> NearChunkDirection;

    inline void SetCentre( const ChunkCoordinate& centre )
    {
        m_PrioritizeCoordinate = centre;
    }

    inline void SetStatusValidRange( std::array<int32_t, ChunkStatusSize> range )
    {
        m_StatusJobRemoveRange = range;
        m_MaxRemoveJobRange    = *std::max_element( m_StatusJobRemoveRange.begin( ), m_StatusJobRemoveRange.end( ) );
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
        CleanRunningThread( );
        m_PendingThreads.clear( );
        m_ChunkCache.clear( );
    }

    void AddCoordinateSafe( const ChunkCoordinate& coordinate, ChunkStatus status = ChunkStatus::eFull )
    {
        std::lock_guard lock( m_SafeAddedChunksLock );
        auto&           chunk = m_SafeAddedChunks[ coordinate ];
        chunk                 = std::max( chunk, (ChunkStatusTy) status );
    }

    bool IsChunkLoading( const ChunkCoordinate& coordinate ) const
    {
        if ( auto find_it = m_ChunkCache.find( ToChunkCoordinateHash( coordinate ) ); find_it != m_ChunkCache.end( ) )
        {
            return find_it->second->initializing;
        }
        return false;
    }

    bool IsChunkLoaded( const ChunkCoordinate& coordinate ) const
    {
        if ( auto find_it = m_ChunkCache.find( ToChunkCoordinateHash( coordinate ) ); find_it != m_ChunkCache.end( ) )
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


    [[nodiscard]] inline std::shared_ptr<ChunkTy> GetChunkCacheSafe( const ChunkCoordinate& coordinate )
    {
        std::lock_guard<std::recursive_mutex> chunkLock( m_ChunkCacheLock );
        if ( auto it = m_ChunkCache.find( ToChunkCoordinateHash( coordinate ) ); it != m_ChunkCache.end( ) )
        {
            return it->second;
        }

        return { };
    }

    // This function should only be called when ChunkCache can be confirmed not changing
    [[nodiscard]] inline std::shared_ptr<ChunkTy> GetChunkCacheUnsafe( const ChunkCoordinate& coordinate ) const
    {
        if ( auto it = m_ChunkCache.find( ToChunkCoordinateHash( coordinate ) ); it != m_ChunkCache.end( ) )
        {
            return it->second;
        }

        return { };
    }

    [[nodiscard]] inline bool CanLoadCoordinate( const ChunkCoordinate& coordinate, ChunkStatus targetStatus ) const
    {
        return m_MaxRemoveJobRange == 0 || MaxAxisDistance( m_PrioritizeCoordinate, coordinate ) <= m_StatusJobRemoveRange[ targetStatus ];
    }
};


#endif   // MINECRAFT_VK_CHUNKPOOL_HPP
