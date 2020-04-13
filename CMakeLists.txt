cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

project("zipper-workflow")

set(Hiredis_PREFIX "${PROJECT_SOURCE_DIR}/extern/hiredis" CACHE PATH "The path to hiredis repo")
add_subdirectory(${Hiredis_PREFIX})
add_subdirectory(lbm_adios)
## tests
add_subdirectory(tests)
add_subdirectory(simpleFoam)

