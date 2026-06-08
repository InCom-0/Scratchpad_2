include(cmake/CPM_0.42.1.cmake)


CPMAddPackage(
    URI "gh:cameron314/readerwriterqueue#master"
    EXCLUDE_FROM_ALL TRUE
    NAME readerwriterqueue
)
CPMAddPackage(
    URI "gh:NVIDIA/stdexec#main"
    EXCLUDE_FROM_ALL TRUE
    NAME stdexec
    OPTIONS
    "STDEXEC_BUILD_TESTS OFF"
)
