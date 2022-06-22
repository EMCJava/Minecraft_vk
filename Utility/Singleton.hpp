//
// Created by loys on 6/3/22.
//

#ifndef MINECRAFT_VK_SINGLETON_HPP
#define MINECRAFT_VK_SINGLETON_HPP

#include <cassert>

template <typename T>
class Singleton
{
protected:
    static T* m_Instance;
    Singleton( )
    {
        assert( m_Instance == nullptr );
        m_Instance = static_cast<T*>( this );
    }

public:
    static T& GetInstance( )
    {
        assert( m_Instance != nullptr );
        return *m_Instance;
    };
};

template <class T>
T* Singleton<T>::m_Instance = nullptr;

#endif   // MINECRAFT_VK_SINGLETON_HPP
