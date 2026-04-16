cmake_minimum_required(VERSION 3.14)
project(LosslessProxy VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define UNICODE for Windows GUI API
add_definitions(-DUNICODE -D_UNICODE)

# Fetch ImGui
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

# Source files
set(CORE_SOURCES
    src/core/main.cpp
    src/core/shader_hook.cpp
    src/core/d3d11_hook.cpp
)

set(ADDON_SOURCES
    src/addon/addon_manager.cpp
    src/addon/addon_security.cpp
    src/addon/addon_dependency.cpp
)

set(EVENT_SOURCES
    src/event/event_system.cpp
)

set(HOST_SOURCES
    src/host/host_impl.cpp
)

set(CONFIG_SOURCES
    src/config/config_manager.cpp
)

set(LOG_SOURCES
    src/log/logger.cpp
)

set(GUI_SOURCES
    src/gui/gui_manager.cpp
    src/gui/gui_style.cpp
    src/gui/icon_loader.cpp
    src/gui/tabs/tab_addons.cpp
    src/gui/tabs/tab_settings.cpp
    src/gui/tabs/tab_logs.cpp
    src/gui/tabs/tab_about.cpp
    src/gui/widgets/toggle_switch.cpp
    src/gui/widgets/toast.cpp
    src/gui/widgets/search_bar.cpp
    src/gui/widgets/addon_card.cpp
)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

# Main DLL target
add_library(Lossless SHARED
    ${CORE_SOURCES}
    ${ADDON_SOURCES}
    ${EVENT_SOURCES}
    ${HOST_SOURCES}
    ${CONFIG_SOURCES}
    ${LOG_SOURCES}
    ${GUI_SOURCES}
    ${IMGUI_SOURCES}
)

target_include_directories(Lossless PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(Lossless
    d3d11
    dxgi
    d3dcompiler
    bcrypt
)

# SDK install target (copies headers for addon developers)
install(DIRECTORY sdk/include/lsproxy DESTINATION include)
