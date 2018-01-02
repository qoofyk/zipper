message("building standalone lbm")

add_executable(lbm lbm.c)
add_library(mod_lbm lbm.c)

#target_link_libraries(lbm ${LIBRARIES})
#target_link_libraries(mod_lbm ${LIBRARIES})

foreach(item lbm mod_lbm)

    target_link_libraries(${item} ${MPI_LIBRARIES})

    if(MPI_COMPILE_FLAGS)
        set_target_properties(${item} PROPERTIES COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS}")
    endif()

    if(MPI_LINK_FLAGS)
        set_target_properties(${item} PROPERTIES LINK_FLAGS "${MPI_C_LINK_FLAGS}")
    endif()
endforeach(item)

