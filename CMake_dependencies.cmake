include(cmake/CPM_0.42.1.cmake)

CPMAddPackage(
    URI "gh:InCom-0/sqlite3-cmake#master"
    OPTIONS "BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS}"
    NAME SQLite3
)
CPMAddPackage(
    URI "gh:rbock/sqlpp23#0.67"
    OPTIONS "BUILD_SQLITE3_CONNECTOR ON"
)



CPMAddPackage(
    URI "gh:cameron314/readerwriterqueue#master"
    EXCLUDE_FROM_ALL TRUE
    NAME readerwriterqueue
)
CPMAddPackage(
    URL https://github.com/NVIDIA/stdexec/archive/refs/tags/nvhpc-26.05.tar.gz
    URL_HASH SHA256=9d2396fecd604698c1eae58f0cb6e4517aa727013846240d1a7b2f35e49884dc
    EXCLUDE_FROM_ALL TRUE
    NAME stdexec
    OPTIONS
    "STDEXEC_BUILD_TESTS OFF"
)
