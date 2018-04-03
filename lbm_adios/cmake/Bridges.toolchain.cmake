# module load intel
set(TRANSPORT_LIB "$ENV{WORK}/envs/gcc_mvapich" CACHE PATH "The path to transport libs")
set(CMAKE_C_FLAGS "-O3" CACHE STRING "")
set(CMAKE_CXX_FLAGS "-O3" CACHE STRING "")
set(BOOST_ROOT "$ENV{WORK}/software/install" CACHE PATH "Boost path")
set(DECAF_PREFIX "$ENV{WORK}/software/install" CACHE PATH "decaf path")
