cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

option(use_itac OFF)
option(ITAC_CCLIENT ON)


project("zipper-workflow")

# prefix: hiredis
set(Hiredis_PREFIX "${PROJECT_SOURCE_DIR}/extern/hiredis" CACHE PATH "The path to hiredis repo")
#set(ENABLE_EXAMPLES ON CACHE BOOL "hiredis examples")
add_subdirectory(${Hiredis_PREFIX})

# ebroker
set(Hiredis_INCLUDE_DIRS ${Hiredis_PREFIX})
set(Hiredis_LIBRARIES hiredis) # this is a cmake target

set(EBROKER_INCLUDE_DIRS ${Hiredis_INCLUDE_DIRS})
set(EBROKER_LIBRARIES elastic_broker ${Hiredis_LIBRARIES})

#itac
if(use_itac)
    add_definitions(-DV_T)
    set(VT_ROOT $ENV{VT_ROOT} CACHE PATH "The path vt")
    message("itac enabled! with ${VT_ROOT}")
    include_directories(${VT_ROOT}/include)
    ### see VT_ADD_LIBS
    #set(vt_library ${INTEL_VT}  $ENV{VT_ADD_LIBS})
    set(vt_library -L$ENV{VT_SLIB_DIR} -lVT $ENV{VT_ADD_LIBS} CACHE STRING "itac link flags")
   #set(common_library ${common_library} "-L/opt/intel/itac/2017.3.030/intel64/lib -lVT -ldwarf -lelf -lvtunwind" )
endif(use_itac)
message("EBROKER_LIBRARI: ${EBROKER_LIBRARIES}")


include_directories(src)

find_package(MPI REQUIRED)


#add_subdirectory(lbm_adios)
## tests
add_subdirectory(src/c-clients/)
add_subdirectory(tests)

#if(FALSE)
if(DEFINED ENV{FOAM_APPBIN})
  add_subdirectory(simpleFoam)
else()
  message(WARNING "openfoam not installed, not building simpleFoam")
  message(WARNING "openfoam $ENV{FOAM_APPBIN}")

endif()


