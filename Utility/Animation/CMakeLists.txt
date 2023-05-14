add_library(AnimationLib Animation.hpp Animation.cpp)
add_library(AnimationLinearLib AnimationLinear.hpp AnimationLinear.cpp)

target_link_libraries(AnimationLinearLib AnimationLib)
