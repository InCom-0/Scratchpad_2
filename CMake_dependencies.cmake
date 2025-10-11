include(FetchContent)

# Get sqlite3
# FetchContent_Declare(
#     sqlite3
#     GIT_REPOSITORY https://github.com/InCom-0/sqlite3-cmake
#     GIT_TAG v3.50.2
#     EXCLUDE_FROM_ALL
# )
    
# FetchContent_MakeAvailable(sqlite3)

# Write a fake package config so that subprojects find our SQLite that's defined by sqlite3-cmake
# file(WRITE "${sqlite3_BINARY_DIR}/SQLite3Config.cmake" 
# "if(NOT TARGET SQLite::SQLite3)
#   add_library(SQLite::SQLite3 ALIAS sqlite3)
# endif()")
# set(SQLite3_DIR "${sqlite3_BINARY_DIR}")


# Now bring in sqlpp23
# FetchContent_Declare(
#     sqlpp23
#     GIT_REPOSITORY  https://github.com/rbock/sqlpp23
#     GIT_TAG         0.67
#     EXCLUDE_FROM_ALL
# )
# set(BUILD_SQLITE3_CONNECTOR ON)
# set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
# FetchContent_MakeAvailable(sqlpp23)
# set(CMAKE_FIND_PACKAGE_PREFER_CONFIG OFF)


## ICU
# set(BUILD_ICU OFF CACHE INTERNAL "")
# set(ICU_STATIC ON CACHE INTERNAL "")
# set(ICU_CFG_OPTIONS "--enable-all" CACHE INTERNAL "")
# set(ICU_BUILD_VERSION 77.1)
# FetchContent_Declare(
#     icu
#     GIT_REPOSITORY https://github.com/viaduck/icu-cmake
#     OVERRIDE_FIND_PACKAGE
#     EXCLUDE_FROM_ALL
# )

set(ICU_NO_INSTALL ON CACHE INTERNAL "")
FetchContent_Declare(
    icu-cpm
    GIT_REPOSITORY https://github.com/InCom-0/icu-cpm
    GIT_TAG        cpm
)

FetchContent_MakeAvailable(icu-cpm)
add_library(ICU::uc ALIAS icu)  # this alias is needed by harfbuzz
add_library(icuuc ALIAS icu)  # this alias is needed by sfntly


FetchContent_Declare(
  sfntly
  GIT_REPOSITORY https://github.com/InCom-0/sfntly
  GIT_TAG main
  SOURCE_SUBDIR cpp
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(sfntly)