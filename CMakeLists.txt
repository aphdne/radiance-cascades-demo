cmake_minimum_required(VERSION 3.31)

set(CMAKE_C_STANDARD            23)
set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED NO)
set(CMAKE_CXX_EXTENSIONS        OFF)

project(radiance_cascades)

find_package(raylib 3.0 REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp src/game.cpp)
target_link_libraries(${PROJECT_NAME} raylib)

if (APPLE)
  target_link_libraries(${PROJECT_NAME} "-framework IOKit")
  target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
  target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
