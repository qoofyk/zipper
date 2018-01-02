if(CMAKE_BUILD_TYPE MATCHES Debug)
  message("-- ${CMAKE_CURRENT_SOURCE_DIR} > Debug build.")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
  message("-- ${CMAKE_CURRENT_SOURCE_DIR} > Release build.")

elseif(CMAKE_BUILD_TYPE MATCHES Stampede)
  message("-- ${CMAKE_CURRENT_SOURCE_DIR} > Stampede uninstructed build")
  set(CMAKE_C_COMPILER  icc)
  set(CMAKE_CXX_COMPILER  icpc)
  set(TRANSPORT_LIB "$ENV{WORK}/envs/transports_icc_impi" CACHE PATH "The path to transport libs.")
  set(CMAKE_C_FLAGS "-O3 -Wall -Wextra -xCORE-AVX2 -axMIC-AVX512" CACHE STRING "cflags")

elseif(CMAKE_BUILD_TYPE MATCHES Deb_Stam)
  message("-- ${CMAKE_CURRENT_SOURCE_DIR} > Stampede debug  build")
  set(CMAKE_C_COMPILER  icc)
  set(CMAKE_CXX_COMPILER  icpc)
  set(TRANSPORT_LIB "$ENV{WORK}/envs/transports_icc_impi" CACHE PATH "The path to transport libs.")
  set(CMAKE_C_FLAGS "-O0 -g -DEBUG_f${ADD_FLAGS}" CACHE STRING "cflags")


elseif(CMAKE_BUILD_TYPE MATCHES Stampede_TAU)
  message("-- ${CMAKE_CURRENT_SOURCE_DIR} > Stampede instructed build")
  set(TRANSPORT_LIB "$ENV{WORK}/envs/transports_icc_impi_tau" CACHE PATH "The path to transport libs.")

elseif(CMAKE_BUILD_TYPE MATCHES Bridges)
  message("-- ${CMAKE_CURRENT_SOURCE_DIR} > Bridges uninstructed build")
  set(TRANSPORT_LIB "$ENV{WORK}/envs/gcc_mvapich" CACHE PATH "The path to transport libs")
  set(CMAKE_C_FLAGS "-O3 ${ADD_FLAGS}" CACHE STRING "cflags")

elseif(CMAKE_BUILD_TYPE MATCHES Bridges_TAU)
  message("-- ${CMAKE_CURRENT_SOURCE_DIR} > Bridges instructed build with tau")
  set(TRANSPORT_LIB "$ENV{WORK}/envs/tau_bundle" CACHE PATH "The path to transport libs")

else()
  message("-- ${CMAKE_CURRENT_SOURCE_DIR} > Defaulting to stampede")
  set(CMAKE_BUILD_TYPE Stampede)
  set(CMAKE_C_COMPILER  icc)
  set(CMAKE_CXX_COMPILER  icpc)
  set(TRANSPORT_LIB "$ENV{WORK}/envs/transports_icc_impi" CACHE PATH "The path to transport libs.")
endif()

message("-- Including transport method in ${TRANSPORT_LIB} is loaded...")

#set(CMAKE_CXX_STANDARD 14)
#set(GCC_COVERAGE_COMPILE_FLAGS "-fPIC -msse3")

#set(CMAKE_CXX_FLAGS_DEBUG "-O2 -g")
#set(CMAKE_C_FLAGS_DEBUG "-O2 -g")
