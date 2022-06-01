//
// Created by loys on 5/23/22.
//

#ifndef MINECRAFT_VK_MINECRAFTTYPE_H
#define MINECRAFT_VK_MINECRAFTTYPE_H

#include <cstdint>
#include <functional>
#include <iostream>
#include <tuple>

using CoordinateType  = int32_t;
using EntityCoordinate = std::tuple<float, float, float>;
using BlockCoordinate = std::tuple<CoordinateType, CoordinateType, CoordinateType>;
using ChunkCoordinate = BlockCoordinate;

inline BlockCoordinate
operator+( const BlockCoordinate& a, const BlockCoordinate& b )
{
    return { get<0>( a ) + get<0>( b ), get<1>( a ) + get<1>( b ), get<2>( a ) + get<2>( b ) };
}

inline BlockCoordinate
operator-( const BlockCoordinate& a, const BlockCoordinate& b )
{
    return { get<0>( a ) - get<0>( b ), get<1>( a ) - get<1>( b ), get<2>( a ) - get<2>( b ) };
}

inline BlockCoordinate
operator-( const BlockCoordinate& a )
{
    return { -get<0>( a ), -get<1>( a ), -get<2>( a ) };
}

inline BlockCoordinate
MakeCoordinate( int x, int y, int z )
{
    return { x, y, z };
}


namespace std
{
namespace
{

    // Code from boost
    // Reciprocal of the golden ratio helps spread entropy
    //     and handles duplicates.
    // See Mike Seymour in magic-numbers-in-boosthash-combine:
    //     http://stackoverflow.com/questions/4948780

    template <class T>
    inline void hash_combine( std::size_t& seed, T const& v )
    {
        seed ^= std::hash<T>( )( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    }

    // Recursive template code derived from Matthieu M.
    template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
    struct HashValueImpl {
        static void apply( size_t& seed, Tuple const& tuple )
        {
            HashValueImpl<Tuple, Index - 1>::apply( seed, tuple );
            hash_combine( seed, std::get<Index>( tuple ) );
        }
    };

    template <class Tuple>
    struct HashValueImpl<Tuple, 0> {
        static void apply( size_t& seed, Tuple const& tuple )
        {
            hash_combine( seed, std::get<0>( tuple ) );
        }
    };

    template <class Ch, class Tr, class TupType, size_t... I>
    void print( std::basic_ostream<Ch, Tr>& os, const TupType& _tup, std::index_sequence<I...> )
    {
        os << "(";
        ( ..., ( os << ( I == 0 ? "" : ", " ) << std::get<I>( _tup ) ) );
        os << ")";
    }
}   // namespace

template <class Ch, class Tr, class... Args>
auto
operator<<( std::basic_ostream<Ch, Tr>& os, std::tuple<Args...> const& t )
    -> std::basic_ostream<Ch, Tr>&
{
    print( os, t, std::make_index_sequence<sizeof...( Args )>( ) );
    return os;
}


template <typename... TT>
struct hash<std::tuple<TT...>> {
    size_t
    operator( )( std::tuple<TT...> const& tt ) const
    {
        size_t seed = 0;
        HashValueImpl<std::tuple<TT...>>::apply( seed, tt );
        return seed;
    }
};
}   // namespace std
#endif   // MINECRAFT_VK_MINECRAFTTYPE_H
