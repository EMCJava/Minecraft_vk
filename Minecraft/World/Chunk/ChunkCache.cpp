//
// Created by loys on 5/24/22.
//

#include "ChunkCache.hpp"

#include <Utility/Logger.hpp>

void
ChunkCache::ResetLoad( )
{
    Logger::getInstance( ).LogLine( Logger::LogType::eInfo, chunk.GetCoordinate( ) );
}
