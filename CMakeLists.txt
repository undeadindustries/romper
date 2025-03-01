cmake_minimum_required(VERSION 3.14)
# Force a universal build (arm64 + x86_64) ONLY on macOS
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
    #set(CMAKE_OSX_ARCHITECTURES arm64 x86_64 CACHE STRING "Universal build" FORCE)
endif()

project(romper)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# Build sqlite3 from the amalgamation files.
add_library(sqlite3 STATIC third_party/sqlite3/amalgamation/sqlite3.c)
target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_COLUMN_METADATA=1)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast")

# Download and build external projects.
include(${CMAKE_CURRENT_LIST_DIR}/cmake/DownloadWxWidgets.cmake)
#include(${CMAKE_CURRENT_LIST_DIR}/cmake/DownloadSQLite3.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/DownloadSQLiteCpp.cmake)

# Create executable target.
if(APPLE)
    add_executable(romper MACOSX_BUNDLE src/main.cpp)
    # Framework linking (e.g. AudioToolbox and WebKit) is handled via wx‑config output.
    set_target_properties(romper PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
    add_custom_command(
        TARGET romper
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:romper>/../Resources"
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/bin/romper_data/romper.romper"
                "$<TARGET_FILE_DIR:romper>/../Resources/romper.romper"
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/icons/romper.icns"
                "$<TARGET_FILE_DIR:romper>/../Resources/romper.icns"
        COMMENT "Copying romper.romper into the app bundle's Resources folder"
    )
elseif(WIN32)
    add_executable(romper WIN32 src/main.cpp main.exe.manifest)
else()
    add_executable(romper src/main.cpp)
endif()

# Retain SQLiteCpp include directory.
target_include_directories(romper PRIVATE
    "${_SQLiteCpp_INCLUDE_DIR}"
)

################################################################################
# Begin wx‑config processing for compile flags (version‑agnostic)
################################################################################

execute_process(
    COMMAND "${CMAKE_SOURCE_DIR}/third_party/wxWidgets_install/bin/wx-config" --cxxflags
    OUTPUT_VARIABLE WX_CXXFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Split the compile flags into a list.
separate_arguments(WX_CXXFLAGS_LIST UNIX_COMMAND "${WX_CXXFLAGS}")

set(wx_include_dirs "")
set(wx_other_flags "")
foreach(flag ${WX_CXXFLAGS_LIST})
    if(flag MATCHES "^-I")
        # Strip the "-I" prefix.
        string(SUBSTRING ${flag} 2 -1 inc_dir)
        list(APPEND wx_include_dirs ${inc_dir})
    else()
        list(APPEND wx_other_flags ${flag})
    endif()
endforeach()


if(APPLE)
    execute_process(
        COMMAND "${CMAKE_SOURCE_DIR}/third_party/wxWidgets_install/bin/wx-config" --version
        OUTPUT_VARIABLE WX_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # Keep only major.minor (e.g. "3.3.0" -> "3.3", "3.3.1" -> "3.3")
    if(WX_VERSION MATCHES "^([0-9]+\\.[0-9]+)")
        set(WX_VERSION ${CMAKE_MATCH_1})
    endif()

    set(WX_SETUP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/wxWidgets_install/lib/wx/include/osx_cocoa-unicode-static-${WX_VERSION}")
    target_include_directories(romper PRIVATE ${WX_SETUP_INCLUDE_DIR})

    add_custom_target(print_wx_setup_include_dir ALL
        COMMAND ${CMAKE_COMMAND} -E echo "WX_SETUP_INCLUDE_DIR = ${WX_SETUP_INCLUDE_DIR}"
    )
endif()

target_include_directories(romper PRIVATE 
${CMAKE_SOURCE_DIR}/third_party/wxWidgets_install/include
${wx_include_dirs})
target_compile_options(romper PRIVATE ${wx_other_flags})


################################################################################
# End wx‑config processing for compile flags
################################################################################

# Ensure external projects are built before this target.
add_dependencies(romper wxWidgets_external SQLiteCpp_external)

# Link libraries from external projects.
# (The wxWidgets linker flags come from DownloadWxWidgets.cmake via wx‑config --libs.)
target_link_libraries(romper PRIVATE ${wxWidgets_LIBRARIES} SQLiteCpp sqlite3)

set_target_properties(romper PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# AppImage packaging for Linux.
set(APPIMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/romper.AppDir")
add_custom_target(appimage
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${APPIMAGE_DIR}"
)