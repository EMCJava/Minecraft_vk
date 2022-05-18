cmake_minimum_required(VERSION 3.20)
project(Minecraft_vk CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
# set(BUILD_SHARED_LIBS ON)
# set(CMAKE_EXE_LINKER_FLAGS "-static")

include_directories(.)

find_package(Threads)
find_package(Vulkan REQUIRED)

add_executable(Minecraft_vk main.cpp)

if (WIN32 OR MSVC)

    find_package(glfw3 CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)

    if (MSVC)
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    endif ()

    set(SHADERC_LIB "shaderc_shared")
    set(WINDOW_PLATFORM_LIB "opengl32;gdi32")
    set(GLFW_PLATFORM_LIB glfw)
    message(STATUS "Assume platform window")

else ()
    find_package(glfw3 3.3 REQUIRED)
    include_directories(${glfw3_INCLUDE_DIRS})

    set(SHADERC_LIB "shaderc_shared")
    set(WINDOW_PLATFORM_LIB "dl;X11;Xxf86vm;Xrandr;Xi;dl")
    set(GLFW_PLATFORM_LIB "glfw")
    message(STATUS "Assume platform linux")

endif ()

include_directories(${Vulkan_INCLUDE_DIRS})

add_subdirectory(Include)
add_subdirectory(Utility)
add_subdirectory(Graphic)
add_subdirectory(Minecraft)

target_link_libraries(Minecraft_vk MainApplicationLib)