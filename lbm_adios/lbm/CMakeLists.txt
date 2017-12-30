message("building lbm")

add_executable(run_lbm run_lbm.c)
add_library(mod_run_lbm run_lbm.c)

target_link_libraries(run_lbm ${LIBRARIES})
target_link_libraries(mod_run_lbm ${LIBRARIES})

