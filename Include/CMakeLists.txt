add_library(GraphicAPILib GraphicAPI.hpp GraphicAPI.cpp)

set(IMGUI_DIR Imgui)
add_library(ImguiLib ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

set(IMPLOT_DIR implot)
add_library(ImplotLib ${IMPLOT_DIR}/implot.h ${IMPLOT_DIR}/implot.cpp ${IMPLOT_DIR}/implot_internal.h ${IMPLOT_DIR}/implot_items.cpp)
include_directories(${IMPLOT_DIR})
target_link_libraries(ImplotLib ImguiLib)

add_library(ImplotDemoLib ${IMPLOT_DIR}/implot_demo.cpp)
target_link_libraries(ImplotDemoLib ImplotLib)

if (WIN32 OR MSVC)
    target_link_libraries(GraphicAPILib glfw)
    target_link_libraries(ImguiLib glfw)
endif ()