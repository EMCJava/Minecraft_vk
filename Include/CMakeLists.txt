add_library(GraphicAPILib GraphicAPI.hpp GraphicAPI.cpp)

set(IMGUI_DIR Imgui)
add_library(ImguiLib ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

if (WIN32 OR MSVC)
    target_link_libraries(GraphicAPILib glfw)
    target_link_libraries(ImguiLib glfw)
endif ()