cmake_minimum_required(VERSION 3.21)
project(Minecraft_vk)

set(CMAKE_CXX_STANDARD 20)

include_directories(.)

find_package(glfw3 3.3 REQUIRED)
find_package(Threads)

add_executable(Minecraft_vk main.cpp)

add_subdirectory(Utility)
add_subdirectory(Vulkan)

target_link_libraries(Minecraft_vk glfw vulkan)

target_link_libraries(Minecraft_vk dl X11 Xxf86vm Xrandr Xi)
target_link_libraries(Minecraft_vk Threads::Threads)

target_link_libraries(Minecraft_vk MainApplicationLib)