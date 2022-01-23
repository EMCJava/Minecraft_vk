add_library(ValidationLayerLib ValidationLayer.hpp ValidationLayer.cpp)
add_library(VulkanExtensionLib VulkanExtension.hpp VulkanExtension.cpp)

target_link_libraries(VulkanExtensionLib ${WINDOW_PLATFORM_LIB} Vulkan::Vulkan ${GLFW_PLATFORM_LIB})
