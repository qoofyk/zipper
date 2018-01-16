message("building standalone lbm")

add_library(mod_lbm lbm.c lbm_buffer.c)

#target_link_libraries(mod_lbm ${LIBRARIES})
#target_link_libraries(mod_lbm ${LIBRARIES})

foreach(item mod_lbm)

    target_link_libraries(${item} ${common_library})

    if(MPI_COMPILE_FLAGS)
        set_target_properties(${item} PROPERTIES COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS}")
    endif()

    if(MPI_LINK_FLAGS)
        set_target_properties(${item} PROPERTIES LINK_FLAGS "${MPI_C_LINK_FLAGS}")
    endif()
endforeach(item)

add_executable(lbm test_lbm.c)
target_link_libraries(lbm mod_lbm)

