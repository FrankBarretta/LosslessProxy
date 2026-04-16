cmake_minimum_required(VERSION 3.14)
project(ExampleAddon)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-DUNICODE -D_UNICODE)

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

add_library(ExampleAddon SHARED main.cpp)

target_include_directories(ExampleAddon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${imgui_SOURCE_DIR}
)
