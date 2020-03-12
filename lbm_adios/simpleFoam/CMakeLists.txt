# see https://cfd.direct/openfoam/user-guide/v6-compiling-applications/
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)                                    
project(simpleFoam)

# this is openfoam cavaty generator
set(OpenFoamRoot "/opt/openfoam6/" CACHE PATH "The path to Openfoam.")                                
set(OpenFoamLib "${OpenFoamRoot}/platforms/linux64GccDPInt32Opt/lib" CACHE PATH "The path to Openfoam lib")                                

set(ENV{WM_PRECISION_OPTION} WM_DP)
add_definitions(-DWM_LABEL_SIZE=32 -DWM_DP)
link_directories(${OpenFoamLib})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories("${OpenFoamRoot}/src/OpenFOAM/lnInclude")
include_directories("${OpenFoamRoot}/src/OSspecific/POSIX/lnInclude")

## used for finiteVolume
#include_directories("${OpenFoamRoot}/src/triSurface/lnInclude")

include_directories("${OpenFoamRoot}/src/TurbulenceModels/turbulenceModels/lnInclude")
include_directories("${OpenFoamRoot}/src/TurbulenceModels/incompressible/lnInclude")
include_directories("${OpenFoamRoot}/src/TurbulenceModels/incompressible/lnInclude")
include_directories("${OpenFoamRoot}/src/transportModels")
include_directories("${OpenFoamRoot}/src/transportModels/incompressible/singlePhaseTransportModel")
include_directories("${OpenFoamRoot}/src/finiteVolume/lnInclude")
include_directories("${OpenFoamRoot}/src/meshTools/lnInclude")
include_directories("${OpenFoamRoot}/src/sampling/lnInclude")

add_executable(mySimpleFoam mySimpleFoam.C)

set(simpleFoamLibs turbulenceModels 
    incompressibleTurbulenceModels
    incompressibleTransportModels
    finiteVolume
    meshTools
    fvOptions
    sampling)

# a toy simulator
#add_executabl(sim_gen sim_gen.c)
#target_link_libraries(sim_gen ds_adaptor)

target_link_libraries(mySimpleFoam turbulenceModels incompressibleTurbulenceModels incompressibleTransportModels finiteVolume meshTools fvOptions sampling OpenFOAM dl  m)