add_library(RenderableChunkLib RenderableChunk.hpp RenderableChunk.cpp)
add_library(WorldChunkLib WorldChunk.cpp WorldChunk.hpp WorldChunk_Impl.hpp)
add_library(ChunkPoolLib ChunkPool.hpp ChunkPool.cpp)
add_library(ChunkLib Chunk.hpp Chunk.cpp)

get_property(StructureLib DIRECTORY ${CMAKE_SOURCE_DIR}/Minecraft/World/Generation/Structure PROPERTY StructureLib)
target_link_libraries(ChunkLib ${StructureLib})
target_link_libraries(ChunkPoolLib RenderableChunkLib WorldChunkLib)
target_link_libraries(WorldChunkLib RenderableChunkLib)
target_link_libraries(RenderableChunkLib ChunkLib)