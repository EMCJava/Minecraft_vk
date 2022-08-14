//
// Created by samsa on 8/9/2022.
//

#ifndef MINECRAFT_VK_UTILITY_RESOURCES_CACHES_HPP
#define MINECRAFT_VK_UTILITY_RESOURCES_CACHES_HPP

#include <array>
#include <concepts>
#include <functional>

template <std::integral auto Size, typename KeyTy, typename ValueTy>
struct Caches {

    std::array<std::pair<KeyTy, ValueTy>, Size> Caches;
    uint32_t                                    CacheIndex = 0;

    ValueTy& Insert( const std::pair<KeyTy, ValueTy>& value )
    {
        CacheIndex = ( CacheIndex + 1 ) % Size;
        return ( Caches[ CacheIndex ] = value ).second;
    }

    void Fill( const std::pair<KeyTy, ValueTy>& value )
    {
        for ( auto& cache : Caches )
            cache = value;
    }

    bool Exists( const KeyTy& key )
    {
        for ( uint32_t i = CacheIndex; i < Size; ++i )
            if ( Caches[ i ].first == key ) return true;
        for ( uint32_t i = 0; i < Size; ++i )
            if ( Caches[ i ].first == key ) return true;

        return false;
    }

    ValueTy& Get( const KeyTy& key, std::function<ValueTy( const KeyTy& )>& fallback )
    {
        for ( uint32_t i = CacheIndex; i < Size; ++i )
            if ( Caches[ i ].first == key ) return Caches[ i ].second;
        for ( uint32_t i = 0; i < Size; ++i )
            if ( Caches[ i ].first == key ) return Caches[ i ].second;

        return Insert( { key, fallback( key ) } );
    }
};

#endif   // MINECRAFT_VK_UTILITY_RESOURCES_CACHES_HPP
