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
getNearChunkDirection<EWDirFront>( )
{
    return MakeMinecraftChunkCoordinate( 1, 0 );
}

template <>
constexpr ChunkCoordinate
getNearChunkDirection<EWDirBack>( )
{
    return MakeMinecraftChunkCoordinate( -1, 0 );
}

template <>
constexpr ChunkCoordinate
getNearChunkDirection<EWDirRight>( )
{
    return MakeMinecraftChunkCoordinate( 0, 1 );
}

template <>
constexpr ChunkCoordinate
getNearChunkDirection<EWDirLeft>( )
{
    return MakeMinecraftChunkCoordinate( 0, -1 );
}

template <>
constexpr ChunkCoordinate
getNearChunkDirection<EWDirFrontRight>( )
{
    return getNearChunkDirection<EWDirFront>( ) + getNearChunkDirection<EWDirRight>( );
}

template <>
constexpr ChunkCoordinate
getNearChunkDirection<EWDirBackLeft>( )
{
    return getNearChunkDirection<EWDirBack>( ) + getNearChunkDirection<EWDirLeft>( );
}

template <>
constexpr ChunkCoordinate
getNearChunkDirection<EWDirFrontLeft>( )
{
    return getNearChunkDirection<EWDirFront>( ) + getNearChunkDirection<EWDirLeft>( );
}

template <>
constexpr ChunkCoordinate
getNearChunkDirection<EWDirBackRight>( )
{
    return getNearChunkDirection<EWDirBack>( ) + getNearChunkDirection<EWDirRight>( );
}

constexpr std::array<ChunkCoordinate, EightWayDirectionSize> NearChunkDirection { getNearChunkDirection<0>( ),
                                                                                  getNearChunkDirection<1>( ),
                                                                                  getNearChunkDirection<2>( ),
                                                                                  getNearChunkDirection<3>( ),
                                                                                  getNearChunkDirection<4>( ),
                                                                                  getNearChunkDirection<5>( ),
                                                                                  getNearChunkDirection<6>( ),
                                                                                  getNearChunkDirection<7>( ) };
}   // namespace


void
ChunkPool::UpdateThread( const std::stop_token& st )
{
    while ( !st.stop_requested( ) )
    {
        if ( !HasThreadRunning( ) )
        {
            // all thread ended, resources are safe to use
            CleanUpJobs( );
            FlushSafeAddedChunks( );
            // RemoveChunkOutsizeRange( );

            if ( m_PendingThreads.empty( ) )
            {
                // std::this_thread::yield();
                std::this_thread::sleep_for( std::chrono::milliseconds( ChunkThreadDelayPeriod ) );
                continue;
            }

            UpdateSorted( [ this ]( ChunkTy* cache ) { ChunkPool::LoadChunk( cache ); },
                          [ centre = m_PrioritizeCoordinate ]( const ChunkTy* a, const ChunkTy* b ) -> bool {
                              const auto aUpgradeable = a->NextStatusUpgradeSatisfied( );
                              const auto bUpgradeable = b->NextStatusUpgradeSatisfied( );
                              if ( aUpgradeable != bUpgradeable ) return aUpgradeable;

                              const auto aEmergency = a->GetEmergencyLevel( );
                              const auto bEmergency = b->GetEmergencyLevel( );
                              if ( aEmergency != bEmergency )
                                  return aEmergency < bEmergency;

                              return a->ManhattanDistance( centre ) < b->ManhattanDistance( centre );
                          } );
        }
    }

    uint32_t leftRunningThread = 0;
    while ( ( leftRunningThread = GetRunningThreadCount( ) ) != 0 )
    {
        Logger::getInstance( ).LogLine( Logger::LogType::eInfo, "Waiting", leftRunningThread, "thread to be finished" );
        CleanRunningThread( );
        std::this_thread::sleep_for( std::chrono::milliseconds( ChunkThreadDelayPeriod ) );
    }
}

void
ChunkPool::RemoveChunkOutsizeRange( )
{
    if ( m_RemoveJobAfterRange > 0 )
    {
        //        std::lock_guard<std::recursive_mutex> lock( m_ChunkCacheLock );
        //        std::lock_guard<std::recursive_mutex> guard( m_PendingThreadsMutex );
        const auto lastIt = std::partition( m_PendingThreads.begin( ), m_PendingThreads.end( ),
                                            [ range = m_RemoveJobAfterRange << 1, centre = m_PrioritizeCoordinate ]( const auto& cache ) { return cache->ManhattanDistance( centre ) <= range; } );

        const auto amountToRemove = std::distance( lastIt, m_PendingThreads.end( ) );

        // elements outside the range
        if ( amountToRemove > 0 )
            m_PendingThreads.erase( lastIt, m_PendingThreads.end( ) );

        std::for_each( m_ChunkCache.begin( ), m_ChunkCache.end( ), [ range = m_RemoveJobAfterRange << 1, centre = m_PrioritizeCoordinate ]( const auto& cache ) {
            if ( cache.second->ManhattanDistance( centre ) > range && ( !cache.second->initializing || cache.second->initialized ) )
            {
                Logger ::getInstance( ).LogLine( Logger::LogType::eInfo, "About to erase chunk outside range", cache.first, cache.second.get( ) );
            }
        } );

        if ( std::erase_if( m_ChunkCache, [ range = m_RemoveJobAfterRange << 1, centre = m_PrioritizeCoordinate ]( const auto& cache ) { return cache.second->ManhattanDistance( centre ) > range && ( !cache.second->initializing || cache.second->initialized ); } ) > 0 )
        {
            m_ChunkErased.test_and_set( );
        }
    }
}

void
ChunkPool::CleanUpJobs( )
{
    std::vector<ChunkTy*> finished;
    finished.reserve( m_maxThread );

    CleanRunningThread( &finished );
    for ( auto* cache : finished )
    {
        if ( !cache->IsAtLeastTargetStatus( ) )
        {
            // Logger::getInstance( ).LogLine( "Load not complete, re-appending chunk", cache );
            cache->initializing = false;
            cache->initialized  = false;
            AddJobContext( cache );
            continue;
        }

        if ( cache->GetStatus( ) == ChunkStatus::eFull )
        {
            for ( int i = 0; i < EightWayDirectionSize; ++i )
            {
                auto chunkPtr = MinecraftServer::GetInstance( ).GetWorld( ).GetChunkPool( ).GetChunkCacheUnsafe( cache->GetChunkCoordinate( ) + NearChunkDirection[ i ] );
                if ( chunkPtr != nullptr && chunkPtr->initialized && chunkPtr->GetStatus( ) == ChunkStatus::eFull )
                {
                    // this is fast, I guess? (nope)

                    // this is ok, I guess
                    // std::lock_guard<std::mutex> lock( m_RenderBufferLock );
                    if ( cache->SyncChunkFromDirection( chunkPtr.get( ), static_cast<EightWayDirection>( i ) ) ) cache->GenerateRenderBuffer( );
                    if ( chunkPtr->SyncChunkFromDirection( cache, static_cast<EightWayDirection>( i ^ 0b1 ) ) ) chunkPtr->GenerateRenderBuffer( );
                }
            }

            // Logger::getInstance( ).LogLine( "New Chunk with size: ", cache->GetObjectSize( ) );
        }

        /* Initialization include mesh building */
        cache->initializing = false;
        cache->initialized  = true;
    }

    {
        std::unordered_map<ChunkCoordinate, ChunkStatusTy> combinedMap;
        for ( auto* finishedJob : finished )
        {
            const auto chunks = finishedJob->ExtractMissingEssentialChunks( );
            for ( const auto& requiredChunk : chunks )
                combinedMap[ requiredChunk.first ] = std::max( combinedMap[ requiredChunk.first ], requiredChunk.second );
            // AddCoordinateSafe( requiredChunk.first, (ChunkStatus) requiredChunk.second );
        }

        // Logger::getInstance( ).LogLine( "combinedMap:", combinedMap.size( ) );
        for ( const auto& requiredChunk : combinedMap )
            AddCoordinateSafe( requiredChunk.first, (ChunkStatus) requiredChunk.second );
    }
}

ChunkTy*
ChunkPool::AddCoordinate( const ChunkCoordinate& coordinate, ChunkStatus status )
{
    ChunkTy*   newChunk;
    const auto hashedCoordinate = ToChunkCoordinateHash( coordinate );

    {
        std::lock_guard<std::recursive_mutex> chunkLock( m_ChunkCacheLock );
        if ( auto find_it = m_ChunkCache.find( hashedCoordinate ); find_it != m_ChunkCache.end( ) )
        {
            // need upgrade
            if ( find_it->second->GetTargetStatus( ) < status )
            {
                find_it->second->SetExpectedStatus( status );

                // still in queue, just modify the target status
                // if ( !find_it->second->initializing && !find_it->second->initialized )
                // {
                // } else

                if ( find_it->second->initialized )   //  previous job already ended, need to add job for upgrading
                    AddJobContext( find_it->second.get( ) );
            }

            return find_it->second.get( );
        }

        newChunk = new ChunkTy( m_World );
        newChunk->SetCoordinate( coordinate );
        newChunk->SetExpectedStatus( status );

        m_ChunkCache.insert( { hashedCoordinate, std::shared_ptr<ChunkTy>( newChunk ) } );
    }

    AddJobContext( newChunk );
    return newChunk;
}

void
ChunkPool::FlushSafeAddedChunks( )
{
    if ( m_SafeAddedChunks.empty( ) ) return;
    std::unordered_map<ChunkCoordinate, ChunkStatusTy> chunks;

    {
        std::lock_guard lock( m_SafeAddedChunksLock );
        chunks = std::move( m_SafeAddedChunks );
    }

    //    std::lock_guard<std::recursive_mutex> chunkLock( m_ChunkCacheLock );
    //    std::lock_guard<std::recursive_mutex> threadLock( m_PendingThreadsMutex );
    for ( const auto& chunk : chunks )
        AddCoordinate( chunk.first, static_cast<ChunkStatus>( chunk.second ) );
}
