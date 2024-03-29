cmake_minimum_required(VERSION 3.20)
project(Minecraft_vk CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set compiler flags for MSVC and g++
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /we4715")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror=uninitialized")
endif()

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

# set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
# set(BUILD_SHARED_LIBS ON)
# set(CMAKE_EXE_LINKER_FLAGS "-static")

include_directories(.)

find_package(Threads)
find_package(Vulkan REQUIRED)
find_package(Boost 1.75.0 COMPONENTS)

add_executable(Minecraft_vk main.cpp)

set_target_properties(
        Minecraft_vk PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

if (WIN32 OR MSVC)

    find_package(glfw3 CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)

    if (MSVC)
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    endif ()

    get_filename_component(Vulkan_Lib_Dir ${Vulkan_LIBRARY} DIRECTORY)

    set(SHADERC_LIB "${Vulkan_Lib_Dir}/shaderc_shared.lib")
    set(WINDOW_PLATFORM_LIB "opengl32;gdi32")
    set(GLFW_PLATFORM_LIB glfw)
    link_libraries(glm::glm)
    message(STATUS "Assume platform window")

else ()
    find_package(glfw3 3.3 REQUIRED)
    find_package(glm REQUIRED)

    set(SHADERC_LIB "shaderc_shared")
    set(WINDOW_PLATFORM_LIB "dl;X11;Xxf86vm;Xrandr;Xi;dl")
    set(GLFW_PLATFORM_LIB "glfw")
    include_directories(${GLM_INCLUDE_DIRS})
    message(STATUS "Assume platform linux")

endif ()

include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(Include/Imgui/)
include_directories(Include)
include_directories(Include/Imgui/backends)
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(Include)
add_subdirectory(Utility)
add_subdirectory(Graphic)
add_subdirectory(Minecraft)

target_link_libraries(Minecraft_vk MainApplicationLib MinecraftWorldLib ChunkPoolLib vkMemAllocImplLib)