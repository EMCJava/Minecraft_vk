#include "QueueFamilyManager.hpp"

#include <Utility/Logger.hpp>

#include <algorithm>
#include <array>

void
QueueFamilyManager::SetupDevice( vk::PhysicalDevice& physicalDevice )
{
    m_queueCounts.clear( );
    m_queueResources.clear( );
    m_vkPhysicalDevice = physicalDevice;

    auto queueFamilies = m_vkPhysicalDevice.getQueueFamilyProperties( );

    std::array<vk::QueueFlagBits, 5> list_of_queue_type {
        vk::QueueFlagBits::eGraphics,
        vk::QueueFlagBits::eCompute,
        vk::QueueFlagBits::eTransfer,
        vk::QueueFlagBits::eSparseBinding,
        vk::QueueFlagBits::eProtected };

    for ( int i = 0; i < queueFamilies.size( ); ++i )
    {
        m_queueCounts.push_back( queueFamilies[ i ].queueCount );
        if ( queueFamilies[ i ].queueCount > 0 )
        {
            for ( const auto& type : list_of_queue_type )
            {
                if ( queueFamilies[ i ].queueFlags & type )
                {
                    m_queueResources[ type ].push_back( i );
                }
            }
        }
    }

    /*
    srand( time( nullptr ) );
    for ( int i = 0; i < 1000000; ++i )
    {
        std::vector<std::pair<uint32_t, uint32_t>> queueCounts;
        for ( int Val = 0; Val < m_queueCounts.size( ); ++Val )
        {
            queueCounts.emplace_back( Val, m_queueCounts[ Val ] );
        }

        std::vector<vk::QueueFlagBits> requestQueue;

        while ( true )
        {
            int randomIndex = rand( ) % queueCounts.size( );
            while ( queueCounts[ randomIndex ].second <= 0 )
            {
                queueCounts.erase( queueCounts.begin( ) + randomIndex );
                if ( queueCounts.empty( ) ) goto finishInitialize;
                randomIndex = rand( ) % queueCounts.size( );
            }

            queueCounts[ randomIndex ].second--;
            auto flag           = static_cast<uint32_t>( queueFamilies[ queueCounts[ randomIndex ].first ].queueFlags );
            auto bitCount       = std::popcount( flag );
            int  randomBitIndex = ( rand( ) % bitCount ) + 1;

            int targetBit = 1;
            while ( true )
            {
                if ( flag & targetBit )
                {
                    if ( --randomBitIndex <= 0 )
                        break;
                }

                targetBit <<= 1;
            }

            requestQueue.push_back( static_cast<const vk::QueueFlagBits>( targetBit ) );
        }
    finishInitialize:

        // std::sort( requestQueue.begin( ), requestQueue.end( ), []( const auto& a, const auto& b ) { return (int) a < (int) b; } );

        GetQueue( requestQueue );

        if ( i % 1000 == 0 )
            Logger::getInstance( ).LogLine( "Finished", i );
    }
    */
}

std::vector<std::pair<uint32_t, uint32_t>>
QueueFamilyManager::GetQueue( const std::vector<vk::QueueFlagBits>& queue )
{
    std::vector<std::pair<uint32_t, uint32_t>> result;

    std::unordered_map<vk::QueueFlagBits, std::vector<std::pair<uint32_t, uint32_t>>> assignedIndex;
    std::unordered_map<vk::QueueFlagBits, uint32_t>                                   queueTypeCount;
    std::vector<uint32_t>                                                             queueCounts( m_queueCounts );


    for ( const auto& element : queue )
        queueTypeCount[ element ]++;

    // <flag, supported by>
    std::vector<std::pair<vk::QueueFlagBits, std::vector<uint32_t>>> elems( m_queueResources.begin( ), m_queueResources.end( ) );

    std::sort( elems.begin( ), elems.end( ), []( const auto& a, const auto& b ) { return a.second.size( ) < b.second.size( ); } );

    while ( !elems.empty( ) )
    {
        // check if flag is needed
        if ( queueTypeCount[ elems.begin( )->first ] <= 0 )
        {
            elems.erase( elems.begin( ) );
            continue;
        }

        bool hasAssigned = false;
        for ( auto& supportedIndex : elems.begin( )->second )
        {
            if ( queueCounts[ supportedIndex ] <= 0 )
                continue;

            assignedIndex[ elems.begin( )->first ].push_back( { supportedIndex, m_queueCounts[ supportedIndex ] - queueCounts[ supportedIndex ] } );
            queueTypeCount[ elems.begin( )->first ]--;
            queueCounts[ supportedIndex ]--;
            hasAssigned = true;
            break;
        }

        if ( !hasAssigned )
            throw std::runtime_error( "Failed to assign queue family!" );
    }

    result.reserve( queue.size( ) );
    for ( const auto& element : queue )
    {
        result.push_back( assignedIndex[ element ].front( ) );
        assignedIndex[ element ].erase( assignedIndex[ element ].begin( ) );
    }

    return result;
}
