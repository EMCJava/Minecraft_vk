cmake_minimum_required(VERSION 3.20)
project(Minecraft_vk CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
# set(BUILD_SHARED_LIBS ON)
# set(CMAKE_EXE_LINKER_FLAGS "-static")

include_directories(.)

find_package(Threads)
find_package(Vulkan REQUIRED)

if (WIN32 OR MSVC)

    find_package(glfw3 CONFIG REQUIRED)

    set(WINDOW_PLATFORM_LIB "opengl32;gdi32")
    message(STATUS "Assume platform window")

else ()
    find_package(glfw3 3.3 REQUIRED)

    set(WINDOW_PLATFORM_LIB "dl;X11;Xxf86vm;Xrandr;Xi;dl;shaderc_shared")
    message(STATUS "Assume platform linux")

endif ()

add_executable(Minecraft_vk main.cpp)

add_subdirectory(Utility)
add_subdirectory(Vulkan)

target_link_libraries(Minecraft_vk MainApplicationLib)