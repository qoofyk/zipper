
set(Hiredis_PREFIX "${PROJECT_SOURCE_DIR}/../deps_install" CACHE PATH "The path to installed hiredis.")
set(Hiredis_INCLUDE_DIRS ${Hiredis_PREFIX}/include/hiredis)
set(Hiredis_LIBRARIES ${Hiredis_PREFIX}/lib/libhiredis.so)
include_directories(${Hiredis_INCLUDE_DIRS})
add_executable(test-hiredis test-hiredis.c)
target_link_libraries(test-hiredis ${Hiredis_LIBRARIES})

add_executable(test-put-mpi test_put_mpi.c)

target_link_libraries(test-put-mpi ${Hiredis_LIBRARIES})

foreach(item test-put-mpi)
    #target_link_libraries(${item} ${MPI_C_LIBRARIES})

    if(MPI_COMPILE_FLAGS)
        set_target_properties(${item} PROPERTIES COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS}")
    endif()

    if(MPI_LINK_FLAGS)
        set_target_properties(${item} PROPERTIES LINK_FLAGS "${MPI_C_LINK_FLAGS}")
    endif()

    target_link_libraries(${item} ${MPI_C_LIBRARIES})
endforeach(item)

