//
// Created by loys on 5/29/22.
//

#ifndef MINECRAFT_VK_BLOCK_HPP
#define MINECRAFT_VK_BLOCK_HPP

enum BlockID {
    Air,
    Stone,
    Grass,
    BlockIDSize
};

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
