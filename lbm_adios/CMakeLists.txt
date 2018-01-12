cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
#include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

project("lbm_adios")
add_definitions ("-DENABLE_TIMING")
#add_definitions ("-DUSE_DATASPACES -DENABLE_TIMING -Wall -Wextra")
#add_definitions ("-DUSE_DATASPACES -DHAS_KEEP -DENABLE_TIMING -Wall -Wextra")
#add_definitions ("-DUSE_DIMES -DENABLE_TIMING -Wall -Wextra")
#add_definitions ("-DUSE_DIMES -DHAS_KEEP -DENABLE_TIMING -Wall -Wextra") 
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH} ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/analysis ${CMAKE_SOURCE_DIR}/msd)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

set(DS_DIR "${TRANSPORT_LIB}/Dataspacesroot" CACHE PATH "The path to dataspaces.")
set(ADIOS_DIR "${TRANSPORT_LIB}/ADIOSroot" CACHE PATH "The path to adios.")
set(FLEXPATH_DIR "${TRANSPORT_LIB}/Flexpath" CACHE PATH "The path to flexpath.")

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
add_library(adaptor adios_adaptor.c utility.c adios_adaptor.c ds_adaptor.c)
target_link_libraries(adaptor ${ADIOS} ${DSPACES} ${DSCOMMON} ${DART} ${EVPATH} ${FFS} ${ATL} ${DILL} ${CERCS_ENV})

# add libm if using gcc
target_link_libraries(adaptor ${MPI_LIBRARIES} pthread rt rdmacm ibverbs)

add_executable(run_lbm run_lbm.c)
add_executable(adios_staging_read adios_staging_read.c)
add_executable(adios_disk_read adios_disk_read.c)

add_executable(native_staging_read native_staging_read.c)
#add_executable(adios_read_global adios_mpiio_read.c run_analysis.c adios_adaptor.c)
# add_executable(adios_write_global adios_write_global.c)

SET(myTARGET run_lbm adios_staging_read adios_disk_read native_staging_read)
foreach(item ${myTARGET})

    target_link_libraries(${item} adaptor m nmoments_analysis)

    if(MPI_COMPILE_FLAGS)
        set_target_properties(${item} PROPERTIES COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS}")
    endif()

    if(MPI_LINK_FLAGS)
        set_target_properties(${item} PROPERTIES LINK_FLAGS "${MPI_C_LINK_FLAGS}")
    endif()
endforeach(item)

add_subdirectory(analysis)
add_subdirectory(msd)
add_subdirectory(lbm)
if(LAMMPS_INCLUDE_DIR)
    add_subdirectory(lammps)
else()
    message("-- lammps not build")
endif()
add_subdirectory(decaf)
add_subdirectory(tests)


