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
    "$ENV{HOME}/.local"
    "$ENV{HOME}/.local/libtorch-cpu/libtorch")
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

# LibTorch (C51 / DQN). Prefer an existing install; optionally download the
# official CPU zip once under ~/.local/libtorch-cpu/libtorch.
option(LIFE_WORLDS_FETCH_LIBTORCH
  "Download LibTorch CPU when find_package(Torch) fails" ON)

set(_lw_libtorch_hint "")
if(WIN32)
  set(_lw_libtorch_hint "$ENV{USERPROFILE}/.local/libtorch-cpu/libtorch")
elseif(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
  set(_lw_libtorch_hint "$ENV{HOME}/.local/libtorch-cpu/libtorch")
endif()
# Normalize to a CMake path (forward slashes) when the hint is set.
if(_lw_libtorch_hint)
  file(TO_CMAKE_PATH "${_lw_libtorch_hint}" _lw_libtorch_hint)
endif()

find_package(Torch QUIET)
if(NOT Torch_FOUND AND LIFE_WORLDS_FETCH_LIBTORCH)
  if(_lw_libtorch_hint AND EXISTS "${_lw_libtorch_hint}/share/cmake/Torch/TorchConfig.cmake")
    list(PREPEND CMAKE_PREFIX_PATH "${_lw_libtorch_hint}")
    find_package(Torch QUIET)
  endif()
endif()

if(NOT Torch_FOUND AND LIFE_WORLDS_FETCH_LIBTORCH)
  set(_lw_torch_version "2.5.1")
  set(_lw_torch_url "")
  if(WIN32)
    set(_lw_torch_url
      "https://download.pytorch.org/libtorch/cpu/libtorch-win-shared-with-deps-${_lw_torch_version}%2Bcpu.zip")
  elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
      set(_lw_torch_url
        "https://download.pytorch.org/libtorch/cpu/libtorch-macos-arm64-${_lw_torch_version}.zip")
    else()
      set(_lw_torch_url
        "https://download.pytorch.org/libtorch/cpu/libtorch-macos-x86_64-${_lw_torch_version}.zip")
    endif()
  elseif(UNIX)
    set(_lw_torch_url
      "https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-${_lw_torch_version}%2Bcpu.zip")
  endif()

  if(_lw_torch_url STREQUAL "" OR _lw_libtorch_hint STREQUAL "")
    message(FATAL_ERROR
      "LibTorch was not found and automatic download is not supported on this "
      "platform. Install LibTorch manually and set CMAKE_PREFIX_PATH, e.g.:\n"
      "  export CMAKE_PREFIX_PATH=\"$(python3 -c 'import torch; print(torch.utils.cmake_prefix_path)')${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}\"")
  endif()

  set(_lw_torch_parent "${_lw_libtorch_hint}/..")
  file(TO_CMAKE_PATH "${_lw_torch_parent}" _lw_torch_parent)
  get_filename_component(_lw_torch_parent "${_lw_torch_parent}" ABSOLUTE)
  set(_lw_torch_zip "${_lw_torch_parent}/libtorch-${_lw_torch_version}-cpu.zip")

  message(STATUS
    "life_worlds: LibTorch not found; downloading CPU ${_lw_torch_version} to "
    "${_lw_libtorch_hint}")
  file(MAKE_DIRECTORY "${_lw_torch_parent}")
  file(DOWNLOAD "${_lw_torch_url}" "${_lw_torch_zip}"
    SHOW_PROGRESS
    STATUS _lw_torch_dl_status
    TLS_VERIFY ON)
  list(GET _lw_torch_dl_status 0 _lw_torch_dl_code)
  list(GET _lw_torch_dl_status 1 _lw_torch_dl_msg)
  if(NOT _lw_torch_dl_code EQUAL 0)
    file(REMOVE "${_lw_torch_zip}")
    message(FATAL_ERROR
      "Failed to download LibTorch (${_lw_torch_dl_msg}).\n"
      "Check the network connection, then either retry configure or install "
      "LibTorch yourself and point CMake at it, e.g.:\n"
      "  export CMAKE_PREFIX_PATH=\"$(python3 -c 'import torch; print(torch.utils.cmake_prefix_path)')${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}\"\n"
      "Or unpack the official CPU zip under ${_lw_libtorch_hint}.\n"
      "Disable auto-download with -DLIFE_WORLDS_FETCH_LIBTORCH=OFF.")
  endif()

  # Official zips contain a top-level "libtorch/" directory.
  file(REMOVE_RECURSE "${_lw_libtorch_hint}")
  file(ARCHIVE_EXTRACT INPUT "${_lw_torch_zip}" DESTINATION "${_lw_torch_parent}")
  file(REMOVE "${_lw_torch_zip}")

  if(NOT EXISTS "${_lw_libtorch_hint}/share/cmake/Torch/TorchConfig.cmake")
    message(FATAL_ERROR
      "LibTorch download finished but TorchConfig.cmake was not found under "
      "${_lw_libtorch_hint}. Install manually, e.g.:\n"
      "  export CMAKE_PREFIX_PATH=\"$(python3 -c 'import torch; print(torch.utils.cmake_prefix_path)')${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}\"")
  endif()

  list(PREPEND CMAKE_PREFIX_PATH "${_lw_libtorch_hint}")
  find_package(Torch QUIET)
endif()

if(NOT Torch_FOUND)
  message(FATAL_ERROR
    "LibTorch was not found.\n"
    "Install a CPU LibTorch and point CMake at it, e.g.:\n"
    "  export CMAKE_PREFIX_PATH=\"$(python3 -c 'import torch; print(torch.utils.cmake_prefix_path)')${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}\"\n"
    "Or unpack the official zip under ${_lw_libtorch_hint}.\n"
    "Or configure with -DLIFE_WORLDS_FETCH_LIBTORCH=ON to download it "
    "(requires network).")
endif()

message(STATUS "life_worlds: Torch ${Torch_VERSION} at ${TORCH_INSTALL_PREFIX}")

add_executable(life_worlds
  src/main.cpp
  src/simulation/simulation.cpp
  src/config_loader.cpp
  src/app/grid_compose.cpp
  src/environment/grid/environment.cpp
  src/environment/grid/adapters.cpp
  src/environment/grid/vision.cpp
  src/environment/grid/render_layers/render_layers.cpp
  src/environment/grid/render_layers/cell_canvas.cpp
  src/environment/grid/render_layers/background.cpp
  src/environment/grid/render_layers/residue.cpp
  src/environment/grid/render_layers/plain.cpp
  src/environment/grid/render_layers/level.cpp
  src/environment/grid/render_layers/value_below.cpp
  src/plant/random_plant/process.cpp
  src/plant/random_plant/residue.cpp
  src/plant/energy_storage_plant/process.cpp
  src/agent/observation.cpp
  src/agent/features.cpp
  src/agent/replay_buffer.cpp
  src/agent/ql_agent/process.cpp
  src/agent/ql_agent/q_table.cpp
  src/agent/c51_agent/process.cpp
  src/agent/c51_agent/network.cpp
  src/agent/dqn_agent/process.cpp
  src/agent/dqn_agent/network.cpp
)

target_include_directories(life_worlds PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(life_worlds PRIVATE
  EnTT::EnTT
  nlohmann_json::nlohmann_json
  ${LIFE_WORLDS_RAYLIB_TARGET}
  ${TORCH_LIBRARIES}
)

# LibTorch expects these definitions / RPATH settings on the executable.
set_property(TARGET life_worlds PROPERTY CXX_STANDARD 17)
if(TORCH_CXX_FLAGS)
  target_compile_options(life_worlds PRIVATE ${TORCH_CXX_FLAGS})
endif()


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
