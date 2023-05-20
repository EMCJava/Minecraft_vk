//
// Created by samsa on 6/22/2022.
//

#ifndef MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_MINECRAFTNOISE_HPP
#define MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_MINECRAFTNOISE_HPP

#include <Include/Noise/FastNoiseLite.hpp>
#include <Utility/Singleton.hpp>

#include <iostream>
#include <memory>

class MinecraftNoise : public Noise::FastNoiseLite
{
private:
    union NoiseType
    {
        std::pair<uint64_t, uint64_t>                  seed;
        uint64_t                                       seedArray[ 2 ];
        std::tuple<int32_t, int32_t, int32_t, int32_t> subSeed;
    };

    NoiseType m_Seed { };

    [[nodiscard]] static inline constexpr int GetIntSeed( const NoiseType& seed )
    {
        return std::get<0>( seed.subSeed ) ^ std::get<1>( seed.subSeed ) ^ std::get<2>( seed.subSeed ) ^ std::get<3>( seed.subSeed );
    }

    static inline constexpr uint64_t
    rotl( const uint64_t x, int k )
    {
        return ( x << k ) | ( x >> ( 64 - k ) );
    }


public:
    constexpr explicit MinecraftNoise( const std::pair<uint64_t, uint64_t>& seed = { 0, 0 } )
        : m_Seed { .seed = seed }
        , Noise::FastNoiseLite( GetIntSeed( NoiseType { .seed = seed } ) )
    { }

    constexpr explicit MinecraftNoise( const std::tuple<int32_t, int32_t, int32_t, int32_t>& seed )
        : m_Seed { .subSeed = seed }
        , Noise::FastNoiseLite( GetIntSeed( NoiseType { .subSeed = seed } ) )
    { }

    constexpr MinecraftNoise( const MinecraftNoise& other )
        : m_Seed { .seed = other.m_Seed.seed }
        , Noise::FastNoiseLite( GetIntSeed( NoiseType { .seed = other.m_Seed.seed } ) )
    {
    }

    MinecraftNoise& operator=( const MinecraftNoise& other )
    {
        m_Seed.seed = other.m_Seed.seed;
        Noise::FastNoiseLite::SetSeed( GetIntSeed( m_Seed ) );
        return *this;
    }

    inline constexpr auto& GetSeed( ) const { return m_Seed; }
    inline constexpr auto  CopySeed( ) const { return m_Seed.seed; }
    inline constexpr void  SetSeed( std::pair<uint64_t, uint64_t>&& seed ) { m_Seed.seed = seed; }
    inline constexpr void  SetSeed( std::tuple<int32_t, int32_t, int32_t, int32_t>&& seed ) { m_Seed.subSeed = seed; }

    /*
     *
     * This function will modify the seed
     *
     * */
    inline constexpr uint64_t
    NextUint64( )
    {
        const uint64_t s0     = m_Seed.seedArray[ 0 ];
        uint64_t       s1     = m_Seed.seedArray[ 1 ];
        const uint64_t result = MinecraftNoise::rotl( s0 + s1, 17 ) + s0;

        s1 ^= s0;
        m_Seed.seedArray[ 0 ] = MinecraftNoise::rotl( s0, 49 ) ^ s1 ^ ( s1 << 21 );   // a, b
        m_Seed.seedArray[ 1 ] = MinecraftNoise::rotl( s1, 28 );                       // c

        return result;
    }

    /*
     *
     * Random number in [0, max)
     *
     * */
    inline constexpr uint64_t
    NextUint64( uint64_t max )
    {
        return NextUint64( ) % max;
    }

    inline constexpr double
    NextDouble( )
    {
        return static_cast<double>( NextUint64( ) ) / static_cast<double>( std::numeric_limits<uint64_t>::max( ) );
    }

    inline constexpr MinecraftNoise&
    Jump( ) noexcept
    {
        constexpr std::uint64_t JUMP[] = { 0x2bd7a6a6e99c2ddc, 0x0992ccaf6a6fca05 };

        std::uint64_t s0 = 0;
        std::uint64_t s1 = 0;

        for ( std::uint64_t jump : JUMP )
        {
            for ( int b = 0; b < 64; ++b )
            {
                if ( jump & UINT64_C( 1 ) << b )
                {
                    s0 ^= m_Seed.seedArray[ 0 ];
                    s1 ^= m_Seed.seedArray[ 1 ];
                }

                NextUint64( );
            }
        }

        m_Seed.seedArray[ 0 ] = s0;
        m_Seed.seedArray[ 1 ] = s1;

        return *this;
    }

    inline constexpr void AlterSeed( std::pair<uint64_t, uint64_t>&& delta )
    {
        m_Seed.seed.first += delta.first;
        m_Seed.seed.second += delta.second;
    }

    static inline constexpr MinecraftNoise FromUint64( uint64_t seed )
    {
        uint64_t z1 = ( seed += 0x9e3779b97f4a7c15 );
        z1          = ( z1 ^ ( z1 >> 30 ) ) * 0xbf58476d1ce4e5b9;
        z1          = ( z1 ^ ( z1 >> 27 ) ) * 0x94d049bb133111eb;

        uint64_t z2 = ( seed + 0x9e3779b97f4a7c15 );
        z2          = ( z2 ^ ( z2 >> 30 ) ) * 0xbf58476d1ce4e5b9;
        z2          = ( z2 ^ ( z2 >> 27 ) ) * 0x94d049bb133111eb;
        return MinecraftNoise { std::make_pair( z1 ^ ( z1 >> 31 ), z2 ^ ( z2 >> 31 ) ) };
    }

    bool inline constexpr operator==( const MinecraftNoise& oth ) const
    {
        return m_Seed.seedArray[ 0 ] == oth.m_Seed.seedArray[ 0 ] && m_Seed.seedArray[ 1 ] == oth.m_Seed.seedArray[ 1 ];
    }

    friend auto operator<<( std::ostream& os, const MinecraftNoise& n ) -> std::ostream&
    {
        return os << '[' << n.m_Seed.seed.first << ", " << n.m_Seed.seed.second << ']' << std::flush;
    }
};


#endif   // MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_MINECRAFTNOISE_HPP
