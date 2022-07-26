//
// Created by loys on 7/26/22.
//

#ifndef MINECRAFT_VK_MUTEXRESOURCES_HPP
#define MINECRAFT_VK_MUTEXRESOURCES_HPP

#include <mutex>

template <typename Ty>
struct MutexResources {
    std::lock_guard<std::mutex> lockGuard;
    Ty&                         resources;

    MutexResources( std::mutex& lock, Ty& resources )
        : lockGuard( lock )
        , resources( resources )
    { }
};

template <typename Ty>
MutexResources<Ty>
MakeMutexResources( std::mutex& lock, Ty&& resources )
{
    return MutexResources<Ty>( lock, std::forward<Ty>( resources ) );
}

#endif   // MINECRAFT_VK_MUTEXRESOURCES_HPP
