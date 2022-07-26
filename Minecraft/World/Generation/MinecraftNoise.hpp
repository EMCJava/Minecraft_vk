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

    [[nodiscard]] inline int GetIntSeed( ) const
    {
        return std::get<0>( m_Seed.subSeed ) ^ std::get<1>( m_Seed.subSeed ) ^ std::get<2>( m_Seed.subSeed ) ^ std::get<3>( m_Seed.subSeed );
    }

    static inline uint64_t
    rotl( const uint64_t x, int k )
    {
        return ( x << k ) | ( x >> ( 64 - k ) );
    }


public:
    explicit MinecraftNoise( const std::pair<uint64_t, uint64_t>& seed = { 0, 0 } )
        : m_Seed { .seed = seed }
        , Noise::FastNoiseLite( GetIntSeed( ) )
    { }

    explicit MinecraftNoise( const std::tuple<int32_t, int32_t, int32_t, int32_t>& seed )
        : m_Seed { .subSeed = seed }
        , Noise::FastNoiseLite( GetIntSeed( ) )
    { }

    MinecraftNoise& operator=( const MinecraftNoise& other )
    {
        m_Seed.seed = other.m_Seed.seed;
        return *this;
    }

    inline const auto& GetSeed( ) const { return m_Seed; }
    inline const auto  CopySeed( ) const { return m_Seed.seed; }
    inline void        SetSeed( std::pair<uint64_t, uint64_t>&& seed ) { m_Seed.seed = seed; }
    inline void        SetSeed( std::tuple<int32_t, int32_t, int32_t, int32_t>&& seed ) { m_Seed.subSeed = seed; }

    /*
     *
     * This function will modify the seed
     *
     * */
    inline uint64_t
    NextUint64( )
    {
        const uint64_t s0     = m_Seed.seedArray[ 0 ];
        uint64_t       s1     = m_Seed.seedArray[ 1 ];
        const uint64_t result = s0 + s1;

        s1 ^= s0;
        m_Seed.seedArray[ 0 ] = MinecraftNoise::rotl( s0, 24 ) ^ s1 ^ ( s1 << 16 );   // a, b
        m_Seed.seedArray[ 1 ] = MinecraftNoise::rotl( s1, 37 );                       // c

        return result;
    }

    friend auto operator<<( std::ostream& os, const MinecraftNoise& n ) -> std::ostream&
    {
        return os << '[' << n.m_Seed.seed.first << ", " << n.m_Seed.seed.second << ']' << std::flush;
    }
};


#endif   // MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_MINECRAFTNOISE_HPP
