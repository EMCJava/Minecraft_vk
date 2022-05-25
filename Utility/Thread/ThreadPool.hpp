//
// Created by loys on 5/23/22.
//

#ifndef MINECRAFT_VK_THREADPOOL_HPP
#define MINECRAFT_VK_THREADPOOL_HPP

#if _MSC_VER
#include <Utility/MSVC.hpp>
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
        Context*           context { };
        std::jthread*      thread { };
        std::atomic<bool>* running { };

        operator Context*( ) const
        {
            return context;
        }

        ThreadInstance( ) = default;
        ~ThreadInstance( )
        {
            delete thread;
            delete running;
        }

        ThreadInstance( const ThreadInstance& )     = delete;
        ThreadInstance( ThreadInstance&& ) noexcept = default;

        ThreadInstance& operator=( const ThreadInstance& )     = delete;
        ThreadInstance& operator=( ThreadInstance&& ) noexcept = default;
    };

    struct ThreadInstanceWrapper {

        std::unique_ptr<ThreadInstance> threadInstance;

        ThreadInstanceWrapper( ThreadInstance* threadInstancePtr )
            : threadInstance( threadInstancePtr )
        { }

        operator Context*( ) const
        {
            return threadInstance->context;
        }
    };

protected:
    std::mutex                         m_PendingThreadsMutex;
    std::vector<Context*>              m_PendingThreads;
    std::vector<ThreadInstanceWrapper> m_RunningThreads;

    uint32_t m_maxThread;

    explicit ThreadPool( uint32_t maxThread )
        : m_maxThread( maxThread )
    { }

    static void RunJob( ThreadInstance* threadInstance, const std::function<void( Context* )>& Job )
    {
        threadInstance->running->wait( false );
        Job( threadInstance->context );
        threadInstance->running->store( false );
    }

    //    std::remove_copy_if( m_RunningThreads.begin( ), m_RunningThreads.end( ), )
    //    std::vector<Context*> CleanRunningThread( )
    //    {
    //        std::vector<Context*> finished;
    //        for ( int i = 0; i < m_RunningThreads.size( ); ++i )
    //        {
    //            if ( !m_RunningThreads[ i ]->running->load( ) )
    //            {
    //                finished.push_back( m_RunningThreads[ i ]->context );
    //                m_RunningThreads.erase( m_RunningThreads.begin( ) + i-- );
    //            }
    //        }
    //
    //        return finished;
    //    }

    std::vector<Context*> CleanRunningThread( )
    {
        std::vector<Context*> finished;
        copy_if( m_RunningThreads.begin( ), m_RunningThreads.end( ), std::back_inserter( finished ),
                 []( const auto& data ) { return !data.threadInstance->running->load( ); } );

        const auto remove = std::remove_if( m_RunningThreads.begin( ), m_RunningThreads.end( ),
                                            []( const auto& data ) { return !data.threadInstance->running->load( ); } );
        m_RunningThreads.erase( remove, m_RunningThreads.end( ) );
        return finished;
    }

    std::vector<Context*> UpdateSorted( const std::function<void( Context* )>& Job, const std::function<bool( const Context*, const Context* )>& Comp )
    {
        std::lock_guard<std::mutex> guard( m_PendingThreadsMutex );
        std::vector<Context*>       finished      = CleanRunningThread( );
        size_t                      threadVacancy = m_maxThread - m_RunningThreads.size( );

        threadVacancy = std::min( m_PendingThreads.size( ), threadVacancy );
        if ( threadVacancy > 0 )
        {
            boost::sort::flat_stable_sort( m_PendingThreads.begin( ), m_PendingThreads.end( ), Comp );
            for ( int i = 0; i < threadVacancy; ++i )
            {
                auto* newThread = new ThreadInstance;
                m_RunningThreads.emplace_back( newThread );

                newThread->context = m_PendingThreads[ i ];
                newThread->running = new std::atomic<bool>( false );
                newThread->thread  = new std::jthread( &ThreadPool::RunJob, newThread, Job );
                newThread->running->store( true );
                newThread->running->notify_one( );
            }

            m_PendingThreads.erase( m_PendingThreads.begin( ), m_PendingThreads.begin( ) + threadVacancy );
        }

        return finished;
    }

    //    std::vector<Context*> Update( const std::function<void( Context* )>& Job )
    //    {
    //        std::vector<Context*> finished;
    //        const int32_t         threadVacancy = m_maxThread - m_RunningThreads.size( );
    //
    //        if ( threadVacancy > 0 )
    //        {
    //            finished.reserve( threadVacancy );
    //            for ( int i = 0; i < threadVacancy; ++i )
    //            {
    //                finished.push_back( m_PendingThreads[ i ] );
    //                m_RunningThreads.emplace_back( ThreadInstance { m_PendingThreads[ i ], new std::thread( Job, m_PendingThreads[ i ] ) } );
    //            }
    //
    //            m_PendingThreads.erase( m_PendingThreads.begin( ), m_PendingThreads.begin( ) + threadVacancy );
    //        }
    //
    //        return finished;
    //    }

    void AddJobContext( Context* context )
    {
        std::lock_guard<std::mutex> guard( m_PendingThreadsMutex );
        m_PendingThreads.push_back( context );
    }

public:
    inline size_t PoolSize( )
    {

        return m_RunningThreads.size( ) + m_PendingThreads.size( );
    }
};


#endif   // MINECRAFT_VK_THREADPOOL_HPP
