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

    set(SHADERC_LIB "shaderc_shared")
    set(WINDOW_PLATFORM_LIB "opengl32;gdi32")
    set(GLFW_PLATFORM_LIB "glfw3")
    message(STATUS "Assume platform window")

else ()
    find_package(glfw3 3.3 REQUIRED)

    set(SHADERC_LIB "shaderc_shared")
    set(WINDOW_PLATFORM_LIB "dl;X11;Xxf86vm;Xrandr;Xi;dl")
    set(GLFW_PLATFORM_LIB "glfw")
    message(STATUS "Assume platform linux")

endif ()

add_executable(Minecraft_vk main.cpp)

add_subdirectory(Utility)
add_subdirectory(Graphic)
add_subdirectory(Minecraft)

target_link_libraries(Minecraft_vk MainApplicationLib)