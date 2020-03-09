# mpi
find_package(MPI REQUIRED)
set(common_library ${common_library} ${MPI_C_LIBRARIES})
include_directories(${MPI_INCLUDE_PATH} ${PROJECT_SOURCE_DIR}/lbm_adios 
    ${PROJECT_SOURCE_DIR}/lbm_adios/analysis
    ${PROJECT_SOURCE_DIR}/lbm_adios/msd
    ${PROJECT_SOURCE_DIR}/lbm_adios/lbm)

include_directories(${PROJECT_SOURCE_DIR}/lbm_adios/include)
add_subdirectory(test-socket)
if(LAMMPS_INCLUDE_DIR)
    add_subdirectory(test-lammps)
endif()
add_subdirectory(test-redis-spark/c-clients)
#add_subdirectory(clog_test)
