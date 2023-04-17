cmake_minimum_required(VERSION 3.14)
project(romper)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast")

find_package(wxWidgets REQUIRED COMPONENTS core base net)
find_package(SQLite3)
if(NOT SQLite3_FOUND)
    #message(STATUS,"Sqlite3 NOT FOUND")
    include(DownloadSQLite3)
endif()

find_package(SQLiteCpp QUIET)
if(NOT SQLiteCpp_FOUND)
    #message(STATUS,"SqliteCpp NOT FOUND")
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/DownloadSQLiteCpp.cmake)
endif()

include(${wxWidgets_USE_FILE})
include_directories(${SQLite3_INCLUDE_DIRS} ${SQLiteCpp_INCLUDE_DIRS})


if(APPLE)
    add_executable(romper MACOSX_BUNDLE src/main.cpp)
    set_target_properties(romper PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
elseif(WIN32)
    add_executable(romper WIN32 src/main.cpp main.exe.manifest)
else()
    add_executable(romper src/main.cpp)
endif()

target_link_libraries(romper ${wxWidgets_LIBRARIES} SQLiteCpp ${SQLite3_LIBRARIES})
set_target_properties(romper PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")


#AppImage packaging for Linux
set(APPIMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/romper.AppDir")

add_custom_target(appimage
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${APPIMAGE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${APPIMAGE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${APPIMAGE_DIR}/usr/bin"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${APPIMAGE_DIR}/usr/share/icons/hicolor"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${APPIMAGE_DIR}/usr/share/applications"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/romper.desktop" "${APPIMAGE_DIR}/usr/share/applications"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/bin/romper_data" "${APPIMAGE_DIR}/usr/bin/romper_data"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/icons/hicolor" "${APPIMAGE_DIR}/usr/share/icons/hicolor"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:romper>" "${APPIMAGE_DIR}/usr/bin"
    COMMAND appimagetool "--appdir=${APPIMAGE_DIR}" "--output=appimage"
    DEPENDS romper
)