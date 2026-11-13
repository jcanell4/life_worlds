cmake_minimum_required(VERSION 3.20)
project(life_worlds LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Search prefixes that are not on the default CMake path:
# Homebrew on Apple Silicon, Intel Homebrew, Linuxbrew, and a user prefix.
foreach(_prefix IN ITEMS
    /opt/homebrew
    /usr/local
    /home/linuxbrew/.linuxbrew
    "$ENV{HOME}/.linuxbrew"
    "$ENV{HOME}/.local")
  if(_prefix AND EXISTS "${_prefix}")
    list(APPEND CMAKE_PREFIX_PATH "${_prefix}")
    if(EXISTS "${_prefix}/lib/pkgconfig")
      set(ENV{PKG_CONFIG_PATH} "${_prefix}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    endif()
  endif()
endforeach()

include(FetchContent)

FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG        v3.13.2
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(entt)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json)

# ON: download raylib when no system package is found.
# OFF: fail at configure time instead of fetching.
option(LIFE_WORLDS_FETCH_RAYLIB
  "Download raylib with FetchContent when it is not installed" ON)

set(LIFE_WORLDS_RAYLIB_TARGET "")

find_package(raylib 5.0 QUIET CONFIG)
if(raylib_FOUND AND TARGET raylib)
  set(LIFE_WORLDS_RAYLIB_TARGET raylib)
elseif(raylib_FOUND AND TARGET raylib::raylib)
  set(LIFE_WORLDS_RAYLIB_TARGET raylib::raylib)
endif()

if(NOT LIFE_WORLDS_RAYLIB_TARGET)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(RAYLIB QUIET IMPORTED_TARGET raylib>=5)
    if(RAYLIB_FOUND AND TARGET PkgConfig::RAYLIB)
      set(LIFE_WORLDS_RAYLIB_TARGET PkgConfig::RAYLIB)
    endif()
  endif()
endif()

if(NOT LIFE_WORLDS_RAYLIB_TARGET)
  if(NOT LIFE_WORLDS_FETCH_RAYLIB)
    message(FATAL_ERROR
      "raylib 5.x was not found. Install it (macOS: brew install raylib; "
      "Linux: the distro package libraylib-dev) or configure with "
      "-DLIFE_WORLDS_FETCH_RAYLIB=ON.")
  endif()
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(raylib)
  set(LIFE_WORLDS_RAYLIB_TARGET raylib)
endif()

message(STATUS "life_worlds: ${CMAKE_SYSTEM_NAME} raylib target: ${LIFE_WORLDS_RAYLIB_TARGET}")

add_executable(life_worlds
  src/main.cpp
  src/simulation/simulation.cpp
  src/config_loader.cpp
  src/app/grid_compose.cpp
  src/environment/grid/environment.cpp
  src/environment/grid/adapters.cpp
  src/environment/grid/layers.cpp
  src/plant/random_plant/process.cpp
  src/agent/ql_agent/process.cpp
  src/agent/ql_agent/q_table.cpp
)

target_include_directories(life_worlds PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(life_worlds PRIVATE
  EnTT::EnTT
  nlohmann_json::nlohmann_json
  ${LIFE_WORLDS_RAYLIB_TARGET}
)

# System libraries raylib needs when linked statically. Shared builds already
# record them on the library itself; repeating them is harmless.
if(APPLE)
  target_link_libraries(life_worlds PRIVATE
    "-framework IOKit"
    "-framework Cocoa"
    "-framework OpenGL"
  )
elseif(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(life_worlds PRIVATE Threads::Threads ${CMAKE_DL_LIBS} m)

  find_library(LIFE_WORLDS_LIBRT rt)
  if(LIFE_WORLDS_LIBRT)
    target_link_libraries(life_worlds PRIVATE ${LIFE_WORLDS_LIBRT})
  endif()

  find_package(OpenGL QUIET)
  if(OpenGL_FOUND)
    target_link_libraries(life_worlds PRIVATE OpenGL::GL)
  endif()

  find_package(X11 QUIET)
  if(X11_FOUND)
    target_link_libraries(life_worlds PRIVATE ${X11_LIBRARIES})
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(life_worlds PRIVATE -Wall -Wextra -Wpedantic)
endif()
