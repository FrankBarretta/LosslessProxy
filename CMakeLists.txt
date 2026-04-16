cmake_minimum_required(VERSION 3.15)
project(LSP_Windowed VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# MinHook
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(minhook)

# ImGui (needed for shared context with host)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

# Source files
set(SOURCES
    src/main.cpp
    src/dxgi_proxy.cpp
    src/windowed_state.cpp
)

# Create the DLL
add_library(LSP_Windowed SHARED ${SOURCES})

target_include_directories(LSP_Windowed PRIVATE
    ${MINHOOK_DIR}/include
    ${imgui_SOURCE_DIR}
    # LosslessProxy SDK headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../LosslessProxy/sdk/include
)

# ImGui core sources (no backends - host provides rendering)
target_sources(LSP_Windowed PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

target_link_libraries(LSP_Windowed
    d3d11.lib
    dxgi.lib
    minhook
)

set_target_properties(LSP_Windowed PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/LSP_Windowed"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/LSP_Windowed"
)
