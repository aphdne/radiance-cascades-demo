cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED NO)
set(CMAKE_CXX_EXTENSIONS        OFF)

# major, minor, patch, and _STAGE_ (0 = alpha, 1 = beta, 2 = gamma/release)
project(radiance_cascades VERSION 0.10.10.0)

configure_file(src/config.h.in config.h)

add_executable(${PROJECT_NAME} src/main.cpp src/game.cpp)
target_include_directories(radiance_cascades PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(${PROJECT_NAME} raylib)

# https://github.com/raysan5/raylib/blob/master/projects/CMake/CMakeLists.txt
set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
  endif()
endif()

if (APPLE)
  target_link_libraries(${PROJECT_NAME} "-framework IOKit")
  target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
  target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
