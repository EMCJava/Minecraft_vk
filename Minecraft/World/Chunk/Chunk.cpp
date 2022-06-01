//
// Created by loys on 5/23/22.
//

#include "Chunk.hpp"

Chunk::~Chunk( )
{
    delete[] m_Blocks;
}
