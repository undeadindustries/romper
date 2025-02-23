cmake_minimum_required(VERSION 3.14)
include(ExternalProject)

set(wxWidgets_REPO "https://github.com/wxWidgets/wxWidgets.git")
set(wxWidgets_TAG "master")

# Configure extra options.
set(wx_shared "-DwxBUILD_SHARED=OFF")
set(wx_debug "-DwxBUILD_DEBUG=ON")
set(wx_opengl "-DwxWITH_OPENGL=ON")
set(wx_webrequest "-DwxWITH_WEBREQUEST=ON")

if(APPLE)
    set(wx_os_arch "-DCMAKE_OSX_ARCHITECTURES=arm64")
elseif(UNIX)
    set(wx_gtk3 "-DwxGTK_VERSION=3")
    set(wx_os_arch "")
else()
    set(wx_os_arch "")
    set(wx_gtk3 "")
endif()

ExternalProject_Add(wxWidgets_external
    GIT_REPOSITORY ${wxWidgets_REPO}
    GIT_TAG ${wxWidgets_TAG}
    UPDATE_DISCONNECTED true
    PREFIX "${CMAKE_SOURCE_DIR}/third_party/wxWidgets_build"
    INSTALL_DIR "${CMAKE_SOURCE_DIR}/third_party/wxWidgets_install"
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/third_party/wxWidgets_install
        ${wx_shared}
        ${wx_debug}
        ${wx_opengl}
        ${wx_webrequest}
        ${wx_os_arch}
        ${wx_gtk3}
    INSTALL_COMMAND
        ${CMAKE_COMMAND} --build . --target install &&
        ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/third_party/wxWidgets_install/include/wx-3.3"
            "${CMAKE_SOURCE_DIR}/third_party/wxWidgets_install/include"
)

# Set include and lib directories.
set(wxWidgets_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/wxWidgets_install/lib")
set(wxWidgets_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/wxWidgets_install/include")

if(NOT EXISTS ${wxWidgets_INCLUDE_DIR})
    message(STATUS "wxWidgets install directory not available; using source location for include directories")
    set(wxWidgets_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/wxWidgets_build/src/wxWidgets_external")
endif()

# Query wx-config for linker flags.
execute_process(
    COMMAND "${CMAKE_SOURCE_DIR}/third_party/wxWidgets_install/bin/wx-config" --libs
    OUTPUT_VARIABLE WX_LIBS_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Instead of splitting, simply replace spaces with semicolons so that tokens remain in order.
string(REPLACE " " ";" WX_LIBS_LIST "${WX_LIBS_OUTPUT}")

# Merge "-framework X" into a single token to avoid CMake interpreting "-framework"
# and "X" as two separate libraries.
set(WX_LIBS_PROCESSED "")
list(LENGTH WX_LIBS_LIST len)
math(EXPR last_index "${len} - 1")

set(i 0)
while(i LESS len)
    list(GET WX_LIBS_LIST ${i} token)
    if(token STREQUAL "-framework" AND i LESS last_index)
        math(EXPR i "${i} + 1")
        list(GET WX_LIBS_LIST ${i} framework_name)
        list(APPEND WX_LIBS_PROCESSED "-framework ${framework_name}")
    else()
        list(APPEND WX_LIBS_PROCESSED "${token}")
    endif()
    math(EXPR i "${i} + 1")
endwhile()

set(wxWidgets_LIBRARIES ${WX_LIBS_PROCESSED})

link_directories(${wxWidgets_LIB_DIR})
include_directories(${wxWidgets_INCLUDE_DIR})

message(STATUS "wxWidgets_LIBRARIES: ${wxWidgets_LIBRARIES}")