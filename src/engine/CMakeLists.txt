target_sources(uieditor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/win32_lib.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/timer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/texture.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/scene_renderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/scene.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ray_cast.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine_math.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/logging.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/input_system.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gpu_resources.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/font_renderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/files.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/debug_render.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/camera_system.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_custom.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine_memory.cpp"

)

# target_include_directories(uieditor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
