include(cmake/CPM_0.42.1.cmake)

CPMAddPackage(
    URI "gh:InCom-0/sqlite3-cmake#master"
    OPTIONS "BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS}"
    NAME SQLite3
)
set(BUILD_SQLITE3_CONNECTOR ON)
CPMAddPackage("gh:rbock/sqlpp23#0.67")



CPMAddPackage(
    URL https://github.com/cameron314/readerwriterqueue/archive/refs/tags/v1.0.7.tar.gz
    URL_HASH SHA256=532224ed052bcd5f4c6be0ed9bb2b8c88dfe7e26e3eb4dd9335303b059df6691
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
