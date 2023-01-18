add_library(EntityLib Entity.hpp Entity.cpp)
add_library(PawnEntityLib PawnEntity.hpp PawnEntity.cpp)
add_library(CameraEntityLib CameraEntity.hpp CameraEntity.cpp)

target_link_libraries(EntityLib PawnEntityLib)
target_link_libraries(CameraEntityLib EntityLib)

add_subdirectory(Player)