#cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
#include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

#project("lbm_zipper")
add_definitions ("-DWRITE_ONE_FILE")
add_definitions ("-DCONSUMER_RB_2GB")
add_definitions ("-DMPI_NOKEEP")

##add_definitions ("-DUSE_DATASPACES -DENABLE_TIMING -Wall -Wextra")
##add_definitions ("-DUSE_DATASPACES -DHAS_KEEP -DENABLE_TIMING -Wall -Wextra")
##add_definitions ("-DUSE_DIMES -DENABLE_TIMING -Wall -Wextra")
##add_definitions ("-DUSE_DIMES -DHAS_KEEP -DENABLE_TIMING -Wall -Wextra")
#find_package(MPI REQUIRED)
##include_directories(${MPI_INCLUDE_PATH} ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/analysis ${CMAKE_SOURCE_DIR}/msd)
#include_directories(${MPI_INCLUDE_PATH} ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/lbm )

#set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

add_executable(lbm_concurrent_nokeep_onefile main.c do_thread.c
    run_module_lbm.c
    compute_generator.c compute_sender.c compute_writer.c
    analysis_consumer.c analysis_reader.c analysis_receiver.c analysis_writer.c)


#add_subdirectory(lbm)

SET(myTARGET lbm_concurrent_nokeep_onefile)
foreach(item ${myTARGET})

    target_link_libraries(${item} pthread m mod_lbm ${common_library})

    if(MPI_COMPILE_FLAGS)
        set_target_properties(${item} PROPERTIES COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS}")
    endif()

    if(MPI_LINK_FLAGS)
        set_target_properties(${item} PROPERTIES LINK_FLAGS "${MPI_C_LINK_FLAGS}")
    endif()
endforeach(item)




