include(ExternalProject)

ExternalProject_Add(SQLiteCpp_external
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG master
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    UPDATE_COMMAND ""
)

ExternalProject_Get_Property(SQLiteCpp_external INSTALL_DIR)

add_library(SQLiteCpp STATIC IMPORTED)
set_target_properties(SQLiteCpp PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libSQLiteCpp.a)
add_dependencies(SQLiteCpp SQLiteCpp_external)

include_directories(${INSTALL_DIR}/include)
