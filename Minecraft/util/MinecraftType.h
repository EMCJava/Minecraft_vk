//
// Created by loys on 5/23/22.
//

#ifndef MINECRAFT_VK_MINECRAFTTYPE_H
#define MINECRAFT_VK_MINECRAFTTYPE_H

#include <cstdint>
#include <functional>
#include <iostream>
#include <tuple>

using CoordinateType = int32_t;

static constexpr int MinecraftCoordinateXIndex = 0;
static constexpr int MinecraftCoordinateYIndex = 2;
static constexpr int MinecraftCoordinateZIndex = 1;

using EntityCoordinate = std::tuple<float, float, float>;
using BlockCoordinate  = std::tuple<CoordinateType, CoordinateType, CoordinateType>;
using ChunkCoordinate  = BlockCoordinate;

namespace
{
enum OperatorIndexTy : uint8_t {
    Add,
    Sub,
    Mul,
    Div,
    RShift,
    LShift
};

template <uint8_t OperatorIndex>
struct Operator {
    static inline constexpr auto Apply( const auto& a, const auto& b ) { }
};
template <>
struct Operator<Add> {
    static inline constexpr auto Apply( const auto& a, const auto& b ) { return a + b; }
};
template <>
struct Operator<Sub> {
    static inline constexpr auto Apply( const auto& a, const auto& b ) { return a - b; }
};
template <>
struct Operator<Mul> {
    static inline constexpr auto Apply( const auto& a, const auto& b ) { return a * b; }
};
template <>
struct Operator<Div> {
    static inline constexpr auto Apply( const auto& a, const auto& b ) { return a / b; }
};
template <>
struct Operator<RShift> {
    static inline constexpr auto Apply( const auto& a, const auto& b ) { return a >> b; }
};
template <>
struct Operator<LShift> {
    static inline constexpr auto Apply( const auto& a, const auto& b ) { return a << b; }
};
template <uint8_t OperatorIndex, typename... T1, typename... T2, std::size_t... I>
inline constexpr auto
OperatorApplyToTuple( const std::tuple<T1...>& t1, const std::tuple<T2...>& t2,
                      std::index_sequence<I...> )
{
    return std::tuple { Operator<OperatorIndex>::Apply( std::get<I>( t1 ), std::get<I>( t2 ) )... };
}

template <uint8_t OperatorIndex, typename... T1, typename... T2>
inline constexpr auto
OperatorApplyToTuple( const std::tuple<T1...>& t1, const std::tuple<T2...>& t2 )
{
    // make sure both tuples have the same size
    static_assert( sizeof...( T1 ) == sizeof...( T2 ) );
    return OperatorApplyToTuple<OperatorIndex>( t1, t2, std::make_index_sequence<sizeof...( T1 )> { } );
}
template <uint8_t OperatorIndex, typename... T1, typename T2, std::size_t... I>
inline constexpr auto
OperatorApplyToTuple( const std::tuple<T1...>& t1, const T2& t2,
                      std::index_sequence<I...> )
{
    return std::tuple { Operator<OperatorIndex>::Apply( std::get<I>( t1 ), t2 )... };
}

template <uint8_t OperatorIndex, typename... T1, typename T2>
inline constexpr auto
OperatorApplyToTuple( const std::tuple<T1...>& t1, const T2& t2 )
{
    return OperatorApplyToTuple<OperatorIndex>( t1, t2, std::make_index_sequence<sizeof...( T1 )> { } );
}
}   // namespace

template <typename... T1, typename... T2>
inline constexpr auto
operator+( const std::tuple<T1...>& t1, const std::tuple<T2...>& t2 )
{
    return OperatorApplyToTuple<Add>( t1, t2 );
}

template <typename... T1, typename... T2>
inline constexpr auto
operator-( const std::tuple<T1...>& t1, const std::tuple<T2...>& t2 )
{
    return OperatorApplyToTuple<Sub>( t1, t2 );
}

template <typename... T1, typename... T2>
inline constexpr auto
operator*( const std::tuple<T1...>& t1, const std::tuple<T2...>& t2 )
{
    return OperatorApplyToTuple<Mul>( t1, t2 );
}

template <typename... T1, typename... T2>
inline constexpr auto
operator/( const std::tuple<T1...>& t1, const std::tuple<T2...>& t2 )
{
    return OperatorApplyToTuple<Div>( t1, t2 );
}

template <typename... T1, typename T2>
inline constexpr auto
operator/( const std::tuple<T1...>& t1, const T2& t2 )
{
    return OperatorApplyToTuple<Div>( t1, t2 );
}


template <typename... T1, typename T2>
inline constexpr auto
operator>>( const std::tuple<T1...>& t1, const T2& t2 )
{
    return OperatorApplyToTuple<RShift>( t1, t2 );
}

template <typename... T1, typename T2>
inline constexpr auto
operator<<( const std::tuple<T1...>& t1, const T2& t2 )
{
    return OperatorApplyToTuple<LShift>( t1, t2 );
}

template <typename Ty>
inline std::tuple<Ty, Ty, Ty>
operator-( const std::tuple<Ty, Ty, Ty>& a )
{
    return { -get<0>( a ), -get<1>( a ), -get<2>( a ) };
}

template <typename Ty>
inline auto&
GetMinecraftX( Ty&& a )
{
    return std::get<MinecraftCoordinateXIndex>( a );
}

template <typename Ty>
inline auto&
GetMinecraftY( Ty&& a )
{
    return std::get<MinecraftCoordinateYIndex>( a );
}

template <typename Ty>
inline auto&
GetMinecraftZ( Ty&& a )
{
    return std::get<MinecraftCoordinateZIndex>( a );
}

template <typename Ty = BlockCoordinate>
inline constexpr Ty
MakeMinecraftCoordinate( auto x, auto y, auto z )
{
    return { x, z, y };
}

template <typename Ty>
inline auto
GetHorizontalMinecraftCoordinate( Ty&& a )
{
    return MakeMinecraftCoordinate( std::get<MinecraftCoordinateXIndex>( a ), 0, std::get<MinecraftCoordinateZIndex>( a ) );
}

template <typename Ty = BlockCoordinate>
inline Ty
MakeCoordinate( auto x, auto y, auto z )
{
    return { x, y, z };
}

constexpr inline BlockCoordinate
ToCartesianCoordinate( BlockCoordinate&& coor )
{
    if constexpr ( MinecraftCoordinateXIndex != 0 )
    {
        std::swap( get<MinecraftCoordinateXIndex>( coor ), get<0>( coor ) );
        if constexpr ( MinecraftCoordinateYIndex == 0 )
        {
            if constexpr ( MinecraftCoordinateXIndex != 1 )
            {
                std::swap( get<MinecraftCoordinateXIndex>( coor ), get<1>( coor ) );
            }
        }

        if constexpr ( MinecraftCoordinateZIndex == 0 )
        {
            if constexpr ( MinecraftCoordinateXIndex != 2 )
            {
                std::swap( get<MinecraftCoordinateXIndex>( coor ), get<2>( coor ) );
            }
        }
    } else
    {
        if constexpr ( MinecraftCoordinateYIndex != 1 )
        {
            std::swap( get<MinecraftCoordinateYIndex>( coor ), get<MinecraftCoordinateZIndex>( coor ) );
        }
    }

    return coor;
}

constexpr inline BlockCoordinate
ToMinecraftCoordinate( BlockCoordinate&& coor )
{
    if constexpr ( MinecraftCoordinateXIndex != 0 )
    {
        if constexpr ( MinecraftCoordinateYIndex == 0 )
        {
            if constexpr ( MinecraftCoordinateXIndex != 1 )
            {
                std::swap( get<MinecraftCoordinateXIndex>( coor ), get<1>( coor ) );
            }
        }

        if constexpr ( MinecraftCoordinateZIndex == 0 )
        {
            if constexpr ( MinecraftCoordinateXIndex != 2 )
            {
                std::swap( get<MinecraftCoordinateXIndex>( coor ), get<2>( coor ) );
            }
        }

        std::swap( get<MinecraftCoordinateXIndex>( coor ), get<0>( coor ) );
    } else
    {
        if constexpr ( MinecraftCoordinateYIndex != 1 )
        {
            std::swap( get<MinecraftCoordinateYIndex>( coor ), get<MinecraftCoordinateZIndex>( coor ) );
        }
    }

    return coor;
}

inline BlockCoordinate
ToCartesianCoordinate( const BlockCoordinate& coor )
{
    return ToCartesianCoordinate( BlockCoordinate { coor } );
}

inline BlockCoordinate
ToMinecraftCoordinate( const BlockCoordinate& coor )
{
    return ToMinecraftCoordinate( BlockCoordinate { coor } );
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
