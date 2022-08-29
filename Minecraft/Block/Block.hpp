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
    Stone,
    Grass,
    Dart,
    BedRock,
    AcaciaLog,
    AzaleaLeaves,
    BirchPlanks,
    Barrel,
    CraftingTable,
    BlockIDSize
};

inline std::string
toString( const BlockID& id )
{
    switch ( id )
    {
        SWITCH_TOSTRING( Air )
        SWITCH_TOSTRING( Stone )
        SWITCH_TOSTRING( Grass )
        SWITCH_TOSTRING( Dart )
        SWITCH_TOSTRING( BedRock )
        SWITCH_TOSTRING( AcaciaLog )
        SWITCH_TOSTRING( AzaleaLeaves )
        SWITCH_TOSTRING( BirchPlanks )
        SWITCH_TOSTRING( Barrel )
        SWITCH_TOSTRING( CraftingTable )
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
