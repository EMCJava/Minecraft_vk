# Minecraft written in C++ and vulkan

## Install

### Window

1. Install Visual Studio ( Not code )
2. Install [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
3. Install vcpkg
4. vcpkg install glm:x64-windows
5. vcpkg install glfw3:x64-windows
6. vcpkg install boost:x64-windows
7. vcpkg integrate install
8. Setup your cmake tool chain as suggested by the output above
9. Copy shaderc_shared.lib from your Vulkan installation into lib folder
