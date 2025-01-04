add_subdirectory(raylib)

file(GLOB ImGuiSources imgui/*.h imgui/*.cpp)
add_library(ImGui ${ImGuiSources})

file(GLOB rlImGuiSources rlImGui/*.h rlImGui/*.cpp)
add_library(rlImGui ${rlImGuiSources})
include_directories(rlImGui ${CMAKE_CURRENT_SOURCE_DIR}/imgui ${CMAKE_CURRENT_SOURCE_DIR}/raylib/src)
target_link_libraries(rlImGui ImGui)
