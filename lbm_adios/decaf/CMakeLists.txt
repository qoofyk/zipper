message("Building vector example")

add_executable            (vector_2nodes            vector_2nodes.cpp)
add_library               (mod_vector_2nodes MODULE vector_2nodes.cpp)
target_link_libraries     (vector_2nodes
                            ${libraries})
    # ${bredala_transports} bredala_datamodel manala decaf)

target_link_libraries   (mod_vector_2nodes
                        ${libraries})
    #${bredala_transports} bredala_datamodel manala decaf)

# .SO file extension on Linux/Mac OS
set_target_properties(mod_vector_2nodes PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_vector_2nodes PROPERTIES PREFIX "")


