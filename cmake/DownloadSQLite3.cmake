include(ExternalProject)

set(sqlite3_REPO "https://github.com/sqlite/sqlite.git")
set(sqlite3_TAG "master")  # You can change this to the desired version

ExternalProject_Add(sqlite3_external
    GIT_REPOSITORY ${sqlite3_REPO}
    GIT_TAG ${sqlite3_TAG}
    UPDATE_DISCONNECTED true
    PREFIX "${CMAKE_BINARY_DIR}/external/sqlite3"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND
        ${CMAKE_COMMAND} -E env "CC=${CMAKE_C_COMPILER}"
        "CFLAGS=${CMAKE_C_FLAGS} -DSQLITE_ENABLE_COLUMN_METADATA=1 -DSQLITE_ENABLE_FTS5=1 -DSQLITE_ENABLE_JSON1=1 -DSQLITE_ENABLE_RTREE=1 -DSQLITE_ENABLE_GEOPOLY=1"
        ${CMAKE_MAKE_PROGRAM} sqlite3.c
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND
        ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/install/lib
        ${CMAKE_COMMAND} -E copy sqlite3.o ${CMAKE_BINARY_DIR}/install/lib/libsqlite3.a
        ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/install/include
        ${CMAKE_COMMAND} -E copy sqlite3.h sqlite3ext.h ${CMAKE_BINARY_DIR}/install/include
)

set(sqlite3_LIB_DIR "${CMAKE_BINARY_DIR}/install/lib")
set(sqlite3_INCLUDE_DIR "${CMAKE_BINARY_DIR}/install/include")
set(sqlite3_LIBRARIES "${sqlite3_LIB_DIR}/libsqlite3.a" PARENT_SCOPE)


link_directories(${sqlite3_LIB_DIR})
