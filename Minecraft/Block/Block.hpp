//
// Created by loys on 5/29/22.
//

#ifndef MINECRAFT_VK_BLOCK_HPP
#define MINECRAFT_VK_BLOCK_HPP

#include <string>

#define SWITCH_TOSTRING( X ) \
case X: return #X;

enum BlockID : uint8_t {
    Air,
    DebugBlock,
    Stone,
    Grass,
    Dart,
    BedRock,
    AcaciaLog,
    AzaleaLeaves,
    FloweringAzaleaLeaves,
    BirchPlanks,
    Barrel,
    CraftingTable,
    BirchLog,
    StrippedBirchLog,
    Water,
    Lava,
    BlockIDSize
};

inline std::string
toString( const BlockID& id )
{
    switch ( id )
    {
        SWITCH_TOSTRING( Air )
        SWITCH_TOSTRING( DebugBlock )
        SWITCH_TOSTRING( Stone )
        SWITCH_TOSTRING( Grass )
        SWITCH_TOSTRING( Dart )
        SWITCH_TOSTRING( BedRock )
        SWITCH_TOSTRING( AcaciaLog )
        SWITCH_TOSTRING( AzaleaLeaves )
        SWITCH_TOSTRING( FloweringAzaleaLeaves )
        SWITCH_TOSTRING( BirchPlanks )
        SWITCH_TOSTRING( Barrel )
        SWITCH_TOSTRING( CraftingTable )
        SWITCH_TOSTRING( BirchLog )
        SWITCH_TOSTRING( StrippedBirchLog )
        SWITCH_TOSTRING( Water )
        SWITCH_TOSTRING( Lava )
    case BlockIDSize: break;
    }

    return "undefined";
}

class Block
{
    BlockID id;

public:
    Block( BlockID id = Air )
        : id( id )
    { }

    inline bool Transparent( ) const
    {
        switch ( id )
        {
        case Air:
            return true;
        default:
            return false;
        }
    }

    inline operator BlockID( ) const
    {
        return id;
    }
};


#endif   // MINECRAFT_VK_BLOCK_HPP
