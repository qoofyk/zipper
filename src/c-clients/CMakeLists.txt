set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

include_directories(${MPI_CXX_HEADER_DIR} ${PROJECT_SOURCE_DIR}/lbm_adios ${EBROKER_INCLUDE_DIRS})
#add_executable(test-hiredis test-hiredis.cpp)
#target_link_libraries(test-hiredis ${Hiredis_LIBRARIES})
add_library(elastic_broker elastic_broker.cpp)

add_executable(test-put-mpi test_put_mpi.cpp)
add_executable(test-put-mpi-foam test_put_mpi_foam.cpp)

foreach(item test-put-mpi test-put-mpi-foam)
    #target_link_libraries(${item} ${MPI_C_LIBRARIES})
    #set_target_properties(${item} PROPERTIES COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS} -g")
    #set_target_properties(${item} PROPERTIES LINK_FLAGS "${MPI_C_LINK_FLAGS}")

    target_link_libraries(${item} ${EBROKER_LIBRARIES})
    if(use_itac)
      message("vt library with ${item}")
      target_link_libraries(${item} PUBLIC ${vt_library})
    endif()

    target_link_libraries(${item})
endforeach(item)
