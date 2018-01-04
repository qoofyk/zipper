
message("Building vector example")
set(transport_mpi on CACHE BOOL "enable mpi transport for decaf")
set(transport_cci on CACHE BOOL "enable cci transport for decaf")
#set(transport_file off CACHE BOOL "enable hdf5 file transport for decaf")
set(CCI_PREFIX "/work/04446/tg837458/stampede2/software/install" CACHE PATH "cci path")
set(DECAF_PREFIX "/work/04446/tg837458/stampede2/software/install" CACHE PATH "decaf path")
include(${CMAKE_SOURCE_DIR}/cmake/decaf.cmake)
include_directories(${CMAKE_SOURCE_DIR}/lbm)

add_executable            (vector_2nodes            vector_2nodes.cpp)
add_library               (mod_vector_2nodes MODULE vector_2nodes.cpp)
target_link_libraries     (vector_2nodes
                            mod_lbm
                            nmoments_analysis
                            ${libraries})
    # ${bredala_transports} bredala_datamodel manala decaf)

target_link_libraries   (mod_vector_2nodes
                        mod_lbm
                        ${libraries})
    #${bredala_transports} bredala_datamodel manala decaf)

# .SO file extension on Linux/Mac OS
set_target_properties(mod_vector_2nodes PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_vector_2nodes PROPERTIES PREFIX "")


