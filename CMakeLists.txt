cmake_minimum_required(VERSION 3.20)
project(Asteroid_Hunter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Configuración de la ruta manual (Donde extrajiste el .zip)
set(RAYLIB_PATH "C:/raylib")

# 2. Directorios de inclusión para encontrar raylib.h
include_directories(${RAYLIB_PATH}/include)

# 3. Definición de archivos fuente
set(SOURCES
        asteroid.cpp
        quadtree.cpp
        quadtree.h
)

# 4. Crear el ejecutable
add_executable(Asteroid_Hunter ${SOURCES})

# 5. Vinculación de la librería estática y dependencias de Windows
if(WIN32)
    # Vinculamos directamente el archivo de la librería
    target_link_libraries(Asteroid_Hunter PRIVATE "${RAYLIB_PATH}/lib/libraylib.a")
    # Librerías nativas que raylib necesita en Windows
    target_link_libraries(Asteroid_Hunter PRIVATE winmm gdi32 opengl32 shell32 user32)
else()
    # Configuración genérica para otros sistemas
    target_link_libraries(Asteroid_Hunter PRIVATE raylib)
endif()

target_include_directories(Asteroid_Hunter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})