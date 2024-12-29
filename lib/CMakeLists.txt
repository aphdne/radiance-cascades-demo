file(GLOB ImGuiSources imgui/*.h imgui/*.cpp)
add_library(ImGui ${ImGuiSources})

file(GLOB rlImGuiSources rlImGui/*.h rlImGui/*.cpp)
add_library(rlImGui ${rlImGuiSources})
include_directories(rlImGui ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
target_link_libraries(rlImGui ImGui)
