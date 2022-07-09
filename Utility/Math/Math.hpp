//
// Created by samsa on 6/27/2022.
//

#ifndef MINECRAFT_VK_UTILITY_MATH_MATH_HPP
#define MINECRAFT_VK_UTILITY_MATH_MATH_HPP

#include <concepts>
#include <initializer_list>

template <std::integral auto Val, std::integral auto Base, std::enable_if_t<Base >= 2, decltype( Val )> Power = 0, typename Ty = decltype( Val )>
struct IntLog {
    using SubTy = IntLog<Val / Base, Base, Power + 1, Ty>;
    static constexpr bool perfect_log { SubTy::perfect_log && Val % Base == 0 };
    static constexpr Ty   value { SubTy::value };   // or also decltype(Val) t {Val};
};

template <std::integral auto Base, auto Power, typename Ty>
struct IntLog<1, Base, Power, Ty> {
    static constexpr bool perfect_log { true };
    static constexpr Ty   value { Power };
};

template <std::integral auto FirstSize, std::integral auto SecondSize, typename Ty>
struct Ratio {

    static constexpr Ty ratio { static_cast<Ty>( SecondSize ) / static_cast<Ty>( FirstSize ) };
};

template <std::integral auto FirstSize, std::integral auto SecondSize, typename FallbackTy = float, typename Parm>
inline constexpr auto
ScaleToSecond( Parm&& size )
{

    if constexpr ( FirstSize > SecondSize )
    {
        if constexpr ( FirstSize % SecondSize == 0 )   // divisible
        {
            using FirstLogVal = IntLog<FirstSize / SecondSize, 2>;
            if constexpr ( FirstLogVal::perfect_log && std::is_integral<Parm>::value )   // shiftable
            {
                return size >> FirstLogVal::value;
            } else
            {
                return size * SecondSize / FirstSize;
            }
        } else
        {
            return size * static_cast<FallbackTy>( SecondSize ) / static_cast<FallbackTy>( FirstSize );
        }
    } else if constexpr ( SecondSize > FirstSize )
    {
        if constexpr ( SecondSize % FirstSize == 0 )   // divisible
        {
            using FirstLogVal = IntLog<SecondSize / FirstSize, 2>;
            if constexpr ( FirstLogVal::perfect_log && std::is_integral<Parm>::value )   // shiftable
            {
                return size << FirstLogVal::value;
            } else
            {
                return size * SecondSize / FirstSize;
            }
        } else
        {
            return size * static_cast<FallbackTy>( SecondSize ) / static_cast<FallbackTy>( FirstSize );
        }
    } else
    {
        return size;
    }
}

template <std::integral auto FirstSize, std::integral auto SecondSize, typename FallbackTy = float, typename... Parm>
inline constexpr auto
ScaleToSecond( Parm&&... sizes ) -> std::initializer_list<decltype( ScaleToSecond<FirstSize, SecondSize, FallbackTy>( 0 ) )>
{
    return { ::ScaleToSecond<FirstSize, SecondSize, FallbackTy>( std::forward<Parm>( sizes ) )... };
}


template <std::integral auto FirstSize, std::integral auto SecondSize, typename FallbackTy = float>
struct ScaleToSecond_S {

    template <typename... Parm>
    inline static constexpr auto
    ConvertToSecond( Parm&&... sizes )
    {
        return ::ScaleToSecond<FirstSize, SecondSize, FallbackTy>( std::forward<Parm>( sizes )... );
    }
};


#endif   // MINECRAFT_VK_UTILITY_MATH_MATH_HPP
