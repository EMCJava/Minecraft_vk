//
// Created by samsa on 5/25/2022.
//
#ifndef MINECRAFT_VK_UTILITY_MSVC_HPP
#define MINECRAFT_VK_UTILITY_MSVC_HPP

#include <functional>
#include <memory>

namespace std
{
template <class _Ty>
pair<_Ty*, std::ptrdiff_t>
get_temporary_buffer( std::ptrdiff_t _Count ) noexcept
{   // get raw temporary buffer of up to _Count elements
    _Ty* _Pbuf;

    if ( _Count < 0 )
        _Count = 0;
    for ( _Pbuf = 0; 0 < _Count; _Count /= 2 )
        try
        {   // try to allocate storage
            _Pbuf = (_Ty*) ::operator new( _Count * sizeof( _Ty ) );
            break;
        }
        catch ( ... )
        {   // loop if new fails
        }

    return ( pair<_Ty*, std::ptrdiff_t>( _Pbuf, _Count ) );
}

// TEMPLATE FUNCTION return_temporary_buffer
template <class _Ty>
void
return_temporary_buffer( _Ty* _Pbuf )
{   // delete raw temporary buffer
    ::operator delete( _Pbuf );
}
}   // namespace std

#endif   // MINECRAFT_VK_UTILITY_MSVC_HPP
