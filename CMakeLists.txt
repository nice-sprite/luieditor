# NOTE ON DEPENDENCIES 
# freetype - 
# harfbuzz - 
# these are so annoying to build on windows + cmake, so I dont build them, 
# I download pre-built versions since they are pretty stable/low frequency updates, and most likely I don't care about the updates anyways.
# I want project layout to be clean, so:
# all dll dependencies go in /bin, next to the exe.
# all libs go in             /lib 
# this is to keep things easily organized and make the cmake script easier to write/use
# when building in release, set the output dir to bin_ship along with all the release versions of 3rd party dlls
# when installed on someones PC the program should look like this
# toplevel_folder
# ----bin // contains all .dlls from 3rd parties
# ----resource // contains resources like the app icon 
# ----projects // maybe. avoiding having to copy or rely on anything in the bo3 root directory is a goal
# ----uieditor.exe 
# ----uieditor.dll

cmake_minimum_required(VERSION 3.22)

project(uieditor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin )
# set(CMAKE_C_COMPILER cl)
# set(CMAKE_CXX_COMPILER cl)

if(MSVC)
    set(CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS")
    add_compile_options(/W2 /MP /GR- /Zi /EHsc /utf-8)
else()
endif()

add_compile_definitions(NOMINMAX _CRT_SECURE_NO_WARNINGS)

set(EXTERNAL_LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external_libs")

add_executable(uieditor WIN32 "src/main.cpp")
option(TRACY_ENABLE "" ON)  
option(TRACY_ON_DEMAND  OFF)  
add_subdirectory(external_libs/tracy)
target_link_libraries(uieditor PUBLIC Tracy::TracyClient)


add_subdirectory(${EXTERNAL_LIBS_DIR}/fmt-10.0.0)
add_subdirectory(${EXTERNAL_LIBS_DIR}/imgui)
add_subdirectory(${EXTERNAL_LIBS_DIR}/moodycamel)

add_compile_definitions(
    GLM_FORCE_SWIZZLE 
    GLM_FORCE_LEFT_HANDED 
    GLM_FORCE_DEPTH_ZERO_TO_ONE 
    GLM_FORCE_RADIANS)

set(freetype_DIR ${EXTERNAL_LIBS_DIR}/freetype)
set(freetype_INCLUDE ${freetype_DIR}/include)

set(harfbuzz_DIR ${EXTERNAL_LIBS_DIR}/harfbuzz-win64)
set(harfbuzz_INCLUDE ${harfbuzz_DIR}/include)

set(glm_DIR ${EXTERNAL_LIBS_DIR}/glm)

set(ImFileDialog_DIR ${EXTERNAL_LIBS_DIR}/ImFileDialog)

target_include_directories(uieditor 
    PUBLIC 
    ${glm_DIR} 
    ${harfbuzz_INCLUDE} 
    ${freetype_INCLUDE} 
    ${EXTERNAL_LIBS_DIR})

add_subdirectory(src/engine)
add_subdirectory(src/application)

target_sources(uieditor PRIVATE ${ImFileDialog_DIR}/ImFileDialog.cpp)
target_include_directories(uieditor PUBLIC ${ImFileDialog_DIR})

target_link_directories(uieditor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/bin)

target_link_libraries(uieditor
    PUBLIC
    fmt
    imgui
    d3d11.lib
    dxgi.lib
    d3dcompiler.lib
    dxguid.lib
    dwmapi.lib
    ole32.lib
    kernel32.lib
    xinput.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/harfbuzz.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/freetype.lib
)

