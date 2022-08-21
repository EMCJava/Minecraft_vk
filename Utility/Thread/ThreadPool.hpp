//
// Created by loys on 5/23/22.
//

#ifndef MINECRAFT_VK_THREADPOOL_HPP
#define MINECRAFT_VK_THREADPOOL_HPP

#if _MSC_VER
#    include <Utility/MSVC.hpp>
#endif

#include <boost/sort/sort.hpp>

#include <thread>
#include <vector>

template <typename Context>
class ThreadPool
{
private:
    ThreadPool( const ThreadPool& ) = delete;
    ThreadPool( ThreadPool&& )      = delete;

    ThreadPool& operator=( const ThreadPool& ) = delete;
    ThreadPool& operator=( ThreadPool&& )      = delete;

public:
    struct ThreadInstance {
        Context*      context { };
        std::jthread* thread { };

        operator Context*( ) const
        {
            return context;
        }

        ThreadInstance( ) = default;
        ~ThreadInstance( )
        {
            delete thread;
        }

        ThreadInstance( const ThreadInstance& )     = delete;
        ThreadInstance( ThreadInstance&& ) noexcept = default;

        ThreadInstance& operator=( const ThreadInstance& )     = delete;
        ThreadInstance& operator=( ThreadInstance&& ) noexcept = default;
    };

    struct ThreadInstanceWrapper {

        std::unique_ptr<ThreadInstance> threadInstance { };
        bool                            occupied = false;

        ThreadInstanceWrapper( ) = default;

        ThreadInstanceWrapper( ThreadInstance* threadInstancePtr )
            : threadInstance( threadInstancePtr )
        { }

        operator Context*( ) const
        {
            return threadInstance->context;
        }
    };

protected:
    std::recursive_mutex               m_PendingThreadsMutex;
    std::vector<Context*>              m_PendingThreads;
    std::vector<ThreadInstanceWrapper> m_RunningThreads;

    uint32_t m_maxThread;

    explicit ThreadPool( uint32_t maxThread )
        : m_maxThread( maxThread )
    {
        m_RunningThreads.resize( m_maxThread );
    }

    static void RunJob( ThreadInstanceWrapper& wrapper, const std::function<void( Context* )>& Job )
    {
        Job( wrapper.threadInstance->context );
        wrapper.occupied = false;
    }

    inline uint32_t GetRunningThreadCount( ) const
    {
        int runningThreadCount = 0;
        for ( int i = 0; i < m_maxThread; ++i )
            if ( m_RunningThreads[ i ].occupied ) runningThreadCount++;
        return runningThreadCount;
    }

    inline bool IsAllThreadCompleted( ) const
    {
        for ( int i = 0; i < m_maxThread; ++i )
            if ( m_RunningThreads[ i ].threadInstance != nullptr ) return false;
        return true;
    }

    void CleanRunningThread( std::vector<Context*>* finished = nullptr )
    {
        for ( int i = 0; i < m_RunningThreads.size( ); ++i )
        {
            //                Finished
            if ( m_RunningThreads[ i ].threadInstance != nullptr && !m_RunningThreads[ i ].occupied )
            {
                if ( finished ) finished->push_back( m_RunningThreads[ i ].threadInstance->context );
                m_RunningThreads[ i ].threadInstance.reset( );
            }
        }
    }

    inline bool HasThreadRunning( )
    {
        return std::any_of( m_RunningThreads.begin( ), m_RunningThreads.end( ), []( const ThreadInstanceWrapper& wrapper ) { return wrapper.threadInstance != nullptr && wrapper.occupied; } );
    }

    void UpdateSortedFull( const std::function<void( Context* )>& Job, const std::function<bool( const Context*, const Context* )>& Comp )
    {
        // only run when all ready
        if ( !HasThreadRunning( ) )
        {
            UpdateSorted( Job, Comp );
        }
    }

    void UpdateSorted( const std::function<void( Context* )>& Job, const std::function<bool( const Context*, const Context* )>& Comp )
    {
        // Let child class clean themselves for more control
        // if ( finishedJob ) finishedJob->clear( );
        // CleanRunningThread( finishedJob );

        if ( m_PendingThreads.empty( ) ) return;

        int emptySlot = 0;
        for ( ; emptySlot < m_maxThread; ++emptySlot )
            if ( m_RunningThreads[ emptySlot ].threadInstance == nullptr ) break;

        if ( emptySlot != m_maxThread )
        {
            std::lock_guard<std::recursive_mutex> guard( m_PendingThreadsMutex );
            boost::sort::flat_stable_sort( m_PendingThreads.begin( ), m_PendingThreads.end( ), Comp );

            int newThreadIndex = 0;
            for ( ; emptySlot < m_maxThread; ++emptySlot )
            {
                if ( m_RunningThreads[ emptySlot ].threadInstance != nullptr ) continue;
                if ( newThreadIndex == m_PendingThreads.size( ) ) break;

                auto* newThread                              = new ThreadInstance;
                m_RunningThreads[ emptySlot ].threadInstance = std::unique_ptr<ThreadInstance>( newThread );
                m_RunningThreads[ emptySlot ].occupied       = true;

                newThread->context = m_PendingThreads[ newThreadIndex++ ];
                newThread->thread  = new std::jthread( &ThreadPool::RunJob, std::ref( m_RunningThreads[ emptySlot ] ), Job );
            }

            m_PendingThreads.erase( m_PendingThreads.begin( ), std::next( m_PendingThreads.begin( ), newThreadIndex ) );
        }
    }

    void AddJobContext( Context* context )
    {
        std::lock_guard<std::recursive_mutex> guard( m_PendingThreadsMutex );
        m_PendingThreads.push_back( context );
    }

public:
    inline size_t PoolSize( )
    {

        return m_RunningThreads.size( ) + m_PendingThreads.size( );
    }

    inline size_t GetMaxThread( )
    {
        return m_maxThread;
    }
};


#endif   // MINECRAFT_VK_THREADPOOL_HPP
