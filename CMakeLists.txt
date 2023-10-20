cmake_minimum_required(VERSION 3.26)
project(Flat_Shading_GraficaComputador)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(SDL2_INCLUDE_DIR "C:/Libs/include")
set(SDL2_LIB_DIR "C:/Libs/lib/x64")

set(GLM_INCLUDE_DIR "C:/Users/rodas/Desktop/CODING11/Repoitorios_GIT/Flat_Shading_GraficaComputador/glm")

include_directories(${SDL2_INCLUDE_DIR} ${GLM_INCLUDE_DIR})
link_directories(${SDL2_LIB_DIR})


add_executable(Flat_Shading_GraficaComputador main.cpp)

target_link_libraries(Flat_Shading_GraficaComputador SDL2main SDL2)