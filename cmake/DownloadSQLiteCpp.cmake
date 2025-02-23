include(ExternalProject)

if(APPLE)
    set(OS_ARCH_FLAG "-DCMAKE_OSX_ARCHITECTURES=arm64") 
else()
    set(OS_ARCH_FLAG "")
endif()

ExternalProject_Add(SQLiteCpp_external
    PREFIX ${CMAKE_SOURCE_DIR}/third_party/SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG master
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/install
               -DSQLITE3_INCLUDE_DIR:PATH=${CMAKE_SOURCE_DIR}/third_party/sqlite3/amalgamation
               -DSQLITE3_LIBRARY:FILEPATH=${CMAKE_BINARY_DIR}/libsqlite3.a
               ${OS_ARCH_FLAG}
    UPDATE_COMMAND ""
)

ExternalProject_Get_Property(SQLiteCpp_external INSTALL_DIR)

add_library(SQLiteCpp STATIC IMPORTED)
set_target_properties(SQLiteCpp PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/SQLiteCpp/src/SQLiteCpp_external-build/libSQLiteCpp.a")
add_dependencies(SQLiteCpp SQLiteCpp_external)

if(NOT EXISTS ${INSTALL_DIR}/include)
    message(STATUS "SQLiteCpp install directory not available; using source include location")
    set(_SQLiteCpp_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/SQLiteCpp/src/SQLiteCpp_external/include")
else()
    set(_SQLiteCpp_INCLUDE_DIR "${INSTALL_DIR}/include")
endif()

message(STATUS "Using SQLiteCpp include directory: ${_SQLiteCpp_INCLUDE_DIR}")

include_directories(${_SQLiteCpp_INCLUDE_DIR})