add_library(ChunkCacheLib ChunkCache.hpp ChunkCache.cpp)
add_library(ChunkPoolLib ChunkPool.hpp ChunkPool.cpp)
add_library(ChunkLib Chunk.hpp Chunk.cpp)

target_link_libraries(ChunkPoolLib ChunkCacheLib)
target_link_libraries(ChunkCacheLib ChunkLib)