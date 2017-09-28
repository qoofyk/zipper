include_directories(${CMAKE_SOURCE_DIR})

add_executable(test_clog_simple test_clog.c)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH} ${CMAKE_SOURCE_DIR})
add_executable(test_clog_mpi test_clog_mpi.c test_clog_minor.c )
SET(myTARGET test_clog_mpi)
foreach(item ${myTARGET})
    target_link_libraries(${item} ${MPI_LIBRARIES})

    if(MPI_COMPILE_FLAGS)
        set_target_properties(${item} PROPERTIES COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS}")
    endif()

    if(MPI_LINK_FLAGS)
        set_target_properties(${item} PROPERTIES LINK_FLAGS "${MPI_C_LINK_FLAGS}")
    endif()
endforeach(item)

