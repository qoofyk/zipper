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

set(ADIOS_LIB ${ADIOS} ${DSPACES} ${DSCOMMON} ${DART} ${EVPATH} ${FFS} ${ATL} ${DILL} ${CERCS_ENV})

#include_directories("${CMAKE_SOURCE_DIR}/src")     
add_library(adaptor adios_adaptor.c adios_adaptor.c ds_adaptor.c)
target_link_libraries(adaptor ${ADIOS_LIB})

# add libm if using gcc
target_link_libraries(adaptor  pthread rt rdmacm ibverbs)


add_executable(run_lbm run_lbm.c)
target_link_libraries(run_lbm mod_lbm)
add_executable(adios_staging_read adios_staging_read.c)
add_executable(adios_disk_read adios_disk_read.c)

add_executable(native_staging_read native_staging_read.c)
#add_executable(adios_read_global adios_mpiio_read.c run_analysis.c adios_adaptor.c)
# add_executable(adios_write_global adios_write_global.c)

SET(myTARGET run_lbm adios_staging_read adios_disk_read native_staging_read)
foreach(item ${myTARGET})

    target_link_libraries(${item} ${common_library} adaptor nmoments_analysis)

    if(MPI_COMPILE_FLAGS)
        set_target_properties(${item} PROPERTIES COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS}")
    endif()

    if(MPI_LINK_FLAGS)
        set_target_properties(${item} PROPERTIES LINK_FLAGS "${MPI_C_LINK_FLAGS}")
    endif()
endforeach(item)


