add_library(GraphicAPILib GraphicAPI.hpp GraphicAPI.cpp)

if (WIN32 OR MSVC)
    target_link_libraries(GraphicAPILib glfw)
endif ()