cmake_minimum_required(VERSION 3.8)

project(WlCubes VERSION 0.2 LANGUAGES CXX C)
message(STATUS "CMAKE_PROJECT_VERSION: '${CMAKE_PROJECT_VERSION}'")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)

message(STATUS "CMAKE_C_COMPILER_ID: '${CMAKE_C_COMPILER_ID}'")
message(STATUS "CMAKE_CXX_COMPILER_ID: '${CMAKE_CXX_COMPILER_ID}'")

set(CMAKE_CXX_STANDARD 20)

message(STATUS "CMAKE_CXX_STANDARD: '${CMAKE_CXX_STANDARD}'")

set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4146 /wd4200 /wd4312")
else()
    add_compile_options(-Wall -Wextra -fms-extensions)
endif()

include_directories(BEFORE "src/adt")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-class-memaccess")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
endif()

add_executable(
    ${CMAKE_PROJECT_NAME}
    src/main.cc
    src/gl/gl.cc
    src/controls.cc
    src/math.cc
    src/frame.cc
    src/Shader.cc
    src/json/lex.cc
    src/json/parser.cc
    src/gltf/gltf.cc
    src/parser/Binary.cc
    src/Texture.cc
    src/Model.cc
    src/Text.cc
)

if (CMAKE_BUILD_TYPE MATCHES "Release" AND CMAKE_SYSTEM_NAME MATCHES "Windows")
    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY WIN32_EXECUTABLE TRUE)
endif()

if (FPS_COUNTER)
    add_definitions("-DFPS_COUNTER")
endif()
if (LOGS)
    add_definitions("-DLOGS")
endif()
if (MODEL)
    add_definitions("-DMODEL")
endif()
if (GLTF)
    add_definitions("-DGLTF")
endif()

if (CMAKE_BUILD_TYPE MATCHES "Asan")
    set(CMAKE_BUILD_TYPE "Debug")

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    else()

        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3 -fsanitize=undefined -fsanitize=address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3 -fsanitize=undefined -fsanitize=address")

        if (CMAKE_SYSTEM_NAME MATCHES "Windows")
            # doesn't work...
            # target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE clang_rt.asan_cxx-x86_64.lib clang_rt.asan-x86_64.lib)
        else()
            add_link_options(-fsanitize=undefined -fsanitize=address)
        endif()

    endif()
endif()

message(STATUS "CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")

if (CMAKE_BUILD_TYPE MATCHES "Debug")

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267 /wd4101 /wd4200")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmissing-field-initializers -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function")
    endif()

    add_definitions("-DLOGS")
    add_definitions("-DDEBUG")
endif()

message(STATUS "CMAKE_SYSTEM_NAME: '${CMAKE_SYSTEM_NAME}'")
message(STATUS "CMAKE_SOURCE_DIR: '${CMAKE_SOURCE_DIR}'")

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LINUX_PKGS REQUIRED gl egl glesv2 wayland-client wayland-egl wayland-cursor)
    include_directories(BEFORE ${LINUX_PKGS_INCLUDE_DIRS})
    target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${LINUX_PKGS_LIBRARIES})

    set(WLP "./src/platform/wayland")
    set(WLPD "${WLP}/wayland-protocols")
    execute_process(
        COMMAND pkg-config wayland-protocols --variable=pkgdatadir
        OUTPUT_VARIABLE WAYLAND_PROTOCOLS_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND pkg-config --variable=wayland_scanner wayland-scanner
        OUTPUT_VARIABLE WAYLAND_SCANNER
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(XDG_SHELL "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
    set(POINTER_CONSTRAINTS "${WAYLAND_PROTOCOLS_DIR}/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml")
    set(RELATIVE_POINTER "${WAYLAND_PROTOCOLS_DIR}/unstable/relative-pointer/relative-pointer-unstable-v1.xml")

    message(STATUS "WLP: '${WLP}'")
    message(STATUS "WLPD: '${WLPD}'")
    message(STATUS "WAYLAND_PROTOCOLS_DIR: '${WAYLAND_PROTOCOLS_DIR}'")
    message(STATUS "WAYLAND_SCANNER: '${WAYLAND_SCANNER}'")
    message(STATUS "XDG_SHELL: '${XDG_SHELL}'")
    message(STATUS "POINTER_CONSTRAINTS: '${POINTER_CONSTRAINTS}'")
    message(STATUS "RELATIVE_POINTER: '${RELATIVE_POINTER}'")

    execute_process(COMMAND mkdir -p ${CMAKE_SOURCE_DIR}/${WLPD})
    execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${RELATIVE_POINTER}    ${CMAKE_SOURCE_DIR}/${WLPD}/relative-pointer-unstable-v1.h)
    execute_process(COMMAND ${WAYLAND_SCANNER} private-code  ${RELATIVE_POINTER}    ${CMAKE_SOURCE_DIR}/${WLPD}/relative-pointer-unstable-v1.c)
    execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${POINTER_CONSTRAINTS} ${CMAKE_SOURCE_DIR}/${WLPD}/pointer-constraints-unstable-v1.h)
    execute_process(COMMAND ${WAYLAND_SCANNER} private-code  ${POINTER_CONSTRAINTS} ${CMAKE_SOURCE_DIR}/${WLPD}/pointer-constraints-unstable-v1.c)
    execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL}           ${CMAKE_SOURCE_DIR}/${WLPD}/xdg-shell.h)
    execute_process(COMMAND ${WAYLAND_SCANNER} private-code  ${XDG_SHELL}           ${CMAKE_SOURCE_DIR}/${WLPD}/xdg-shell.c)

    target_sources(
        ${CMAKE_PROJECT_NAME}
        PRIVATE
        ${WLP}/WlClient.cc
        ${WLP}/input.cc
        ${WLPD}/pointer-constraints-unstable-v1.c
        ${WLPD}/relative-pointer-unstable-v1.c
        ${WLPD}/xdg-shell.c
    )
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    find_package(OpenGL REQUIRED)
    message(STATUS "OpenGL: '${OpenGL}'")
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${OPENGL_gl_LIBRARY})

    target_sources(
        ${CMAKE_PROJECT_NAME}
        PRIVATE
        src/platform/windows/glad.cc
        src/platform/windows/input.cc
        src/platform/windows/windows.cc
    )
endif()

# install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
