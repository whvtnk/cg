cmake_minimum_required(VERSION 3.16)
project(week10 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

include(FetchContent)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(glm)

add_executable(cg main.cpp external/glad/src/gl.c)
target_include_directories(cg PRIVATE external/glad/include)
target_link_libraries(cg PRIVATE glfw glm::glm OpenGL::GL ${CMAKE_DL_LIBS})

# Windows (MSYS2 MinGW): статическая линковка - cg.exe запускается без DLL из MSYS2
if(MINGW)
    target_link_options(cg PRIVATE -static)
endif()
