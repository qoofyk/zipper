
set(Hiredis_INCLUDE_DIRS ${Hiredis_PREFIX}/hiredis)
set(Hiredis_LIBRARIES hiredis) # this is a cmake target
include_directories(${Hiredis_INCLUDE_DIRS})
add_executable(test-hiredis test-hiredis.c)
target_link_libraries(test-hiredis ${Hiredis_LIBRARIES})

add_executable(test-put-mpi test_put_mpi.cpp)
add_executable(test-put-mpi-foam test_put_mpi_foam.cpp)

foreach(item test-put-mpi test-put-mpi-foam)
    #target_link_libraries(${item} ${MPI_C_LIBRARIES})
    target_link_libraries(${item} ${Hiredis_LIBRARIES})

    if(MPI_CXX_COMPILE_FLAGS)
        set_target_properties(${item} PROPERTIES COMPILE_FLAGS "${MPI_CXX_COMPILE_FLAGS}")
    endif()

    if(MPI_CXX_LINK_FLAGS)
        set_target_properties(${item} PROPERTIES LINK_FLAGS "${MPI_CXX_LINK_FLAGS}")
    endif()

    target_link_libraries(${item} ${MPI_CXX_LIBRARIES})
endforeach(item)

