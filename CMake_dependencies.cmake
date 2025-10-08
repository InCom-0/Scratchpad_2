include(FetchContent)

# Get sqlite3
FetchContent_Declare(
    sqlite3
    GIT_REPOSITORY https://github.com/InCom-0/sqlite3-cmake
    GIT_TAG v3.50.2)
FetchContent_MakeAvailable(sqlite3)

# Write a fake package config so that subprojects find our SQLite that's defined by sqlite3-cmake
file(WRITE "${sqlite3_BINARY_DIR}/SQLite3Config.cmake" 
"if(NOT TARGET SQLite::SQLite3)
  add_library(SQLite::SQLite3 ALIAS sqlite3)
endif()")
set(SQLite3_DIR "${sqlite3_BINARY_DIR}")


# Now bring in sqlpp23
FetchContent_Declare(
    sqlpp23
    GIT_REPOSITORY  https://github.com/rbock/sqlpp23
    GIT_TAG         0.67
)
set(BUILD_SQLITE3_CONNECTOR ON)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
FetchContent_MakeAvailable(sqlpp23)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG OFF)