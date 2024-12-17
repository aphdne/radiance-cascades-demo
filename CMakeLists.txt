cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED NO)
set(CMAKE_CXX_EXTENSIONS        OFF)

# major, minor, patch, and _STAGE_ (0 = alpha, 1 = beta, 2 = gamma/release)
project(radiance_cascades VERSION 0.8.0.0)

configure_file(config.h.in config.h)

find_package(raylib 3.0 REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp src/game.cpp)
target_include_directories(radiance_cascades PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(${PROJECT_NAME} raylib)

if (APPLE)
  target_link_libraries(${PROJECT_NAME} "-framework IOKit")
  target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
  target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
