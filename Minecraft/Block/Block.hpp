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

    inline bool Transparent( )
    {
        switch ( id )
        {
        case Air:
            return true;
        default:
            return false;
        }
    }

    inline operator BlockID( )
    {
        return id;
    }
};


#endif   // MINECRAFT_VK_BLOCK_HPP
