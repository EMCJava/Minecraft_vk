add_library(ChunkCacheLib ChunkCache.hpp ChunkCache.cpp)
add_library(ChunkPoolLib ChunkPool.hpp ChunkPool.cpp)
add_library(ChunkLib Chunk.hpp Chunk.cpp)

get_property(StructureLib DIRECTORY ${CMAKE_SOURCE_DIR}/Minecraft/World/Generation/Structure PROPERTY StructureLib)
target_link_libraries(ChunkLib ${StructureLib})
target_link_libraries(ChunkPoolLib ChunkCacheLib)
target_link_libraries(ChunkCacheLib ChunkLib)