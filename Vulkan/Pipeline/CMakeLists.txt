add_library(VulkanShaderLib VulkanShader.hpp VulkanShader.cpp)


target_link_libraries(VulkanShaderLib ${SHADERC_LIB})

add_library(VulkanPipelineLib VulkanPipeline.hpp VulkanPipeline.cpp)