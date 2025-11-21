cmake_minimum_required(VERSION 3.14)
project(RickAndMortyClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Opción 1: usar CPM.cmake para traer cpr + nlohmann/json
# (queda todo muy limpio en proyectos pequeños)

include(FetchContent)

# ---- CPR ----
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG        1.10.5  # o la versión estable que prefieras
)
set(CPR_ENABLE_SSL ON CACHE BOOL "Enable SSL in CPR" FORCE)
FetchContent_MakeAvailable(cpr)

# ---- nlohmann/json ----
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3  # o similar
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(rickmorty main.cpp)

target_link_libraries(rickmorty
    PRIVATE
        cpr::cpr
        nlohmann_json::nlohmann_json
)

# Para Windows: evitar consola extra si compilas como GUI (no es el caso)
# set_property(TARGET rickmorty PROPERTY WIN32_EXECUTABLE FALSE)
