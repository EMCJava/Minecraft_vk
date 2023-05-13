add_subdirectory(Block)
add_subdirectory(Entity)
add_subdirectory(Internet)
add_subdirectory(World)

add_library(MinecraftLib Minecraft.hpp Minecraft.cpp)
target_link_libraries(MinecraftLib MinecraftServerLib)

add_subdirectory(Application)