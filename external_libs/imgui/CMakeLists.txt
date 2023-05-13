add_library(imgui STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_dx11.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_win32.cpp"
)

target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
