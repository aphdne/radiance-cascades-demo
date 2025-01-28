cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED NO)
set(CMAKE_CXX_EXTENSIONS        OFF)

# major, minor, patch, and _STAGE_ (0 = alpha, 1 = beta, 2 = gamma/release)
project(radiance_cascades VERSION 0.3.0.1)

configure_file(src/config.h.in config.h)

add_executable(${PROJECT_NAME} src/main.cpp src/demo.cpp)

add_subdirectory(lib)
target_include_directories(radiance_cascades PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/lib/rlImGui" "${PROJECT_SOURCE_DIR}/lib/imgui")
target_link_libraries(${PROJECT_NAME} raylib rlImGui ImGui)

# add_compile_definitions(DEBUG)

if (APPLE)
  target_link_libraries(${PROJECT_NAME} "-framework IOKit")
  target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
  target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
