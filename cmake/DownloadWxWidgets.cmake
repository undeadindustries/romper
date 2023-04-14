include(ExternalProject)

set(wxWidgets_REPO "https://github.com/wxWidgets/wxWidgets.git")
set(wxWidgets_TAG "master")

ExternalProject_Add(wxWidgets_external
    GIT_REPOSITORY ${wxWidgets_REPO}
    GIT_TAG ${wxWidgets_TAG}
    UPDATE_DISCONNECTED true
    PREFIX "${CMAKE_BINARY_DIR}/external/wxWidgets"
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
        -DwxBUILD_SHARED=OFF
)

set(wxWidgets_LIB_DIR "${CMAKE_BINARY_DIR}/install/lib")
set(wxWidgets_INCLUDE_DIR "${CMAKE_BINARY_DIR}/install/include")

link_directories(${wxWidgets_LIB_DIR})
include_directories(${wxWidgets_INCLUDE_DIR})

add_dependencies(romper wxWidgets_external)
