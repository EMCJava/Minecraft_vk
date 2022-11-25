//
// Created by samsa on 7/31/2022.
//

#include "StructurePieces.hpp"

size_t
StructurePieces::GetObjectSize( ) const
{
    return sizeof( m_PiecesBoundingBox[ 0 ] ) * m_PiecesBoundingBox.capacity( );
}
