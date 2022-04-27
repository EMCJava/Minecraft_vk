add_subdirectory(Pipeline)

add_library(VulkanAPILib VulkanAPI.hpp VulkanAPI.cpp)
target_link_libraries(VulkanAPILib GraphicAPILib)
