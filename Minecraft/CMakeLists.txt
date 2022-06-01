add_subdirectory(Block)
add_subdirectory(Player)
add_subdirectory(Internet)
add_subdirectory(World)

add_library(MinecraftLib Minecraft.hpp Minecraft.cpp)
add_subdirectory(Application)