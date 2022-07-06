//
// Created by samsa on 6/22/2022.
//

#ifndef MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_MINECRAFTNOISE_HPP
#define MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_MINECRAFTNOISE_HPP

#include <memory>

#include <Include/Noise/FastNoiseLite.hpp>
#include <Utility/Singleton.hpp>

class MinecraftNoise : public Noise::FastNoiseLite
{
public:
    explicit MinecraftNoise( int seed )
        : Noise::FastNoiseLite( seed )
    { }
};


#endif   // MINECRAFT_VK_MINECRAFT_WORLD_GENERATION_MINECRAFTNOISE_HPP
