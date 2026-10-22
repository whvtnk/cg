# Минимальная версия CMake: FetchContent и современные target_* команды требуют 3.14+.
cmake_minimum_required(VERSION 3.14)

# Название проекта и используемые языки (C нужен для glad/src/gl.c, CXX - для остального).
project(TowerBuilder LANGUAGES C CXX)

# Используем стандарт C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Исполняемый файл кладём в build/bin/ (CMAKE_BINARY_DIR - это папка build).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

# ---------------------------------------------------------------- GLFW 3.4
# Отключаем сборку лишнего: примеры, тесты, документацию.
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
# Wayland не собираем (используем X11), как требуется в задании.
set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)

# Скачиваем исходники GLFW нужной версии с GitHub во время конфигурации.
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

# ---------------------------------------------------------------- GLM
# GLM - библиотека только из заголовков. Сначала ищем установленную в системе,
# если не нашли - скачиваем через FetchContent.
find_package(glm QUIET)
if(NOT glm_FOUND)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
    )
    FetchContent_MakeAvailable(glm)
endif()

# ---------------------------------------------------------------- glad
# glad лежит в external/glad/ (include/glad/gl.h, include/KHR/khrplatform.h, src/gl.c).
# Собираем её как маленькую статическую библиотеку.
add_library(glad STATIC external/glad/src/gl.c)
target_include_directories(glad PUBLIC external/glad/include)

# ---------------------------------------------------------------- OpenGL
# Системная библиотека OpenGL (libGL / opengl32).
find_package(OpenGL REQUIRED)

# ---------------------------------------------------------------- Игра
add_executable(tower_builder src/main.cpp)

# Линкуем: окно (glfw), загрузчик функций (glad), математику (glm) и OpenGL.
target_link_libraries(tower_builder PRIVATE glfw glad glm::glm OpenGL::GL)
