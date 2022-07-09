add_library(BlockLib Block.hpp Block.cpp)
add_library(BlockTextureLib BlockTexture.hpp BlockTexture.cpp)

target_link_libraries(BlockTextureLib ImageMetaLib)