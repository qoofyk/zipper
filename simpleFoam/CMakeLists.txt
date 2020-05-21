# see https://cfd.direct/openfoam/user-guide/v6-compiling-applications/
# credit: https://www.cfd-online.com/Forums/openfoam-programming-development/205243-share-something-about-dev-openfoam-using-cmake-rather-than-wmake.html
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(simpleFoam)

if(DEFINED $ENV{FOAM_APP_APPBIN})
set(EXECUTABLE_OUTPUT_PATH $ENV{FOAM_USER_APPBIN})
else()
set(EXECUTABLE_OUTPUT_PATH $ENV{FOAM_APPBIN})
endif()

set( CMAKE_CXX_STANDARD 11)
add_definitions(-DWM_LABEL_SIZE=32 -DWM_DP)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker --no-as-needed -Xlinker --add-needed")
link_directories($ENV{FOAM_LIBBIN})
add_definitions(-Dlinux64 -DWM_ARCH_OPTION=64 -DWM_DP -DWM_LABEL_SIZE=32 -DNoRepository -m64 -fPIC)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories("$ENV{FOAM_SRC}/OpenFOAM/lnInclude")
include_directories("$ENV{FOAM_SRC}/OSspecific/POSIX/lnInclude")

## used for finiteVolume
#include_directories("$ENV{FOAM_SRC}/triSurface/lnInclude")

include_directories("$ENV{FOAM_SRC}/TurbulenceModels/turbulenceModels/lnInclude")
include_directories("$ENV{FOAM_SRC}/TurbulenceModels/incompressible/lnInclude")
include_directories("$ENV{FOAM_SRC}/TurbulenceModels/incompressible/lnInclude")
include_directories("$ENV{FOAM_SRC}/transportModels")
include_directories("$ENV{FOAM_SRC}/transportModels/incompressible/singlePhaseTransportModel")
include_directories("$ENV{FOAM_SRC}/finiteVolume/lnInclude")
include_directories("$ENV{FOAM_SRC}/meshTools/lnInclude")
include_directories("$ENV{FOAM_SRC}/sampling/lnInclude")
include_directories(${CMAKE_SOURCE_DIR}/extern/hiredis)

add_executable(mySimpleFoam mySimpleFoam.C)

add_executable(mySimpleFoamNoWrite mySimpleFoam.C)
target_compile_definitions(mySimpleFoamNoWrite PUBLIC METHOD_NOWRITE)

add_executable(mySimpleFoamCloud mySimpleFoam.C)
target_compile_definitions(mySimpleFoamCloud PUBLIC METHOD_CLOUD)
target_link_libraries(mySimpleFoamCloud hiredis)

add_executable(mySimpleFoamFile mySimpleFoam.C)
target_compile_definitions(mySimpleFoamFile PUBLIC METHOD_FILE)

foreach(target mySimpleFoam mySimpleFoamCloud mySimpleFoamFile mySimpleFoamNoWrite)
target_link_libraries(${target} turbulenceModels incompressibleTurbulenceModels incompressibleTransportModels finiteVolume meshTools fvOptions sampling OpenFOAM dl  m)
endforeach()
