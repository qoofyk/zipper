cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project("lbm_adios")
add_definitions ("-DUSE_DATASPACES -DENABLE_TIMING -Wall -Wextra")
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

set(DS_DIR "$ENV{HOME}/envs/Dataspacesroot" CACHE PATH "The path to dataspaces.")
set(ADIOS_DIR "$ENV{HOME}/envs/ADIOSroot" CACHE PATH "The path to dataspaces.")

include_directories(${DS_DIR}/include ${ADIOS_DIR}/include)

find_library(DSPACES dspaces ${DS_DIR}/lib)
find_library(DSCOMMON dscommon ${DS_DIR}/lib) 
find_library(DART dart ${DS_DIR}/lib) 
find_library(ADIOS adios ${ADIOS_DIR}/lib)

#include_directories("${CMAKE_SOURCE_DIR}/src")     

add_executable(run_lbm run_lbm.c adios_adaptor.c utility.c)
add_executable(adios_read_global adios_staging_read.c run_analysis.c adios_adaptor.c utility.c)
#add_executable(adios_read_global adios_mpiio_read.c run_analysis.c adios_adaptor.c)
# add_executable(adios_write_global adios_write_global.c)
target_link_libraries(run_lbm ${ADIOS} ${DSPACES} ${DSCOMMON} ${DART})
target_link_libraries(adios_read_global ${ADIOS} ${DSPACES} ${DSCOMMON} ${DART})
#target_link_libraries(adios_write_global ${ADIOS} ${DSPACES} ${DSCOMMON} ${DART} )
target_link_libraries(run_lbm ${MPI_LIBRARIES} pthread rt rdmacm ibverbs m)
target_link_libraries(adios_read_global ${MPI_LIBRARIES} pthread rt rdmacm ibverbs m)
#add_subdirectory(manual)

if(MPI_COMPILE_FLAGS)
    set_target_properties(run_lbm PROPERTIES COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS}")
    set_target_properties(adios_read_global PROPERTIES COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(run_lbm PROPERTIES LINK_FLAGS "${MPI_C_LINK_FLAGS}")
    set_target_properties(adios_read_global PROPERTIES LINK_FLAGS "${MPI_C_LINK_FLAGS}")
endif()
