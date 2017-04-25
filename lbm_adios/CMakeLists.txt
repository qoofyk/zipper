cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_C_FLAGS "-O3 ${ADD_FLAGS}" CACHE STRING "cflags")

project("lbm_adios")
add_definitions ("-DENABLE_TIMING -Wall -Wextra")
#add_definitions ("-DUSE_DATASPACES -DENABLE_TIMING -Wall -Wextra")
#add_definitions ("-DUSE_DATASPACES -DHAS_KEEP -DENABLE_TIMING -Wall -Wextra")
#add_definitions ("-DUSE_DIMES -DENABLE_TIMING -Wall -Wextra")
#add_definitions ("-DUSE_DIMES -DHAS_KEEP -DENABLE_TIMING -Wall -Wextra")
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

set(DS_DIR "$ENV{HOME}/envs/Dataspacesroot" CACHE PATH "The path to dataspaces.")
set(ADIOS_DIR "$ENV{HOME}/envs/ADIOSroot" CACHE PATH "The path to adios.")
set(FLEXPATH_DIR "$ENV{HOME}/envs/Flexpath" CACHE PATH "The path to flexpath.")

include_directories(${DS_DIR}/include ${ADIOS_DIR}/include ${FLEXPATH_DIR}/include)

find_library(DSPACES dspaces ${DS_DIR}/lib)
find_library(DSCOMMON dscommon ${DS_DIR}/lib) 
find_library(DART dart ${DS_DIR}/lib) 
find_library(ADIOS adios ${ADIOS_DIR}/lib)

find_library(EVPATH evpath ${FLEXPATH_DIR}/lib)
find_library(FFS ffs ${FLEXPATH_DIR}/lib)
find_library(ATL atl ${FLEXPATH_DIR}/lib)
find_library(DILL dill ${FLEXPATH_DIR}/lib)
find_library(CERCS_ENV cercs_env ${FLEXPATH_DIR}/lib)

#include_directories("${CMAKE_SOURCE_DIR}/src")     

add_executable(run_lbm run_lbm.c adios_adaptor.c utility.c adios_adaptor.c common_utility.c)
add_executable(adios_read_global adios_staging_read.c run_analysis.c adios_adaptor.c utility.c)
add_executable(mpiio_read adios_mpiio_read.c run_analysis.c adios_adaptor.c utility.c)

add_executable(raw_dspaces_read raw_dspaces_read.c run_analysis.c adios_adaptor.c utility.c ds_adaptor.c common_utility.c)
#add_executable(adios_read_global adios_mpiio_read.c run_analysis.c adios_adaptor.c)
# add_executable(adios_write_global adios_write_global.c)

SET(myTARGET run_lbm adios_read_global mpiio_read)
foreach(item ${myTARGET})
    target_link_libraries(${item} ${ADIOS} ${DSPACES} ${DSCOMMON} ${DART} ${EVPATH} ${FFS} ${ATL} ${DILL} ${CERCS_ENV})
    target_link_libraries(${item} ${MPI_LIBRARIES} pthread rt rdmacm ibverbs m)

    if(MPI_COMPILE_FLAGS)
        set_target_properties(${item} PROPERTIES COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS}")
    endif()

    if(MPI_LINK_FLAGS)
        set_target_properties(${item} PROPERTIES LINK_FLAGS "${MPI_C_LINK_FLAGS}")
    endif()
endforeach(item)


