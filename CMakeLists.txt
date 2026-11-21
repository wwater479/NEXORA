cmake_minimum_required(VERSION 3.20)
project(NEXORA VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler options
if(MSVC)
    add_compile_options(/W4 /utf-8 /permissive- /EHsc /MP)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX -DWIN32_LEAN_AND_MEAN)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -pthread)
endif()

# Global include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cef
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cef/include
)

# Third party
add_subdirectory(third_party/cef)

#
# 1. Util Library
#
add_library(nexora_util STATIC
    src/util/logger.cpp
    src/util/string_util.cpp
    src/util/path_util.cpp
    src/util/json_util.cpp
    src/util/win32_util.cpp
)

#
# 2. Storage Library
#
add_library(nexora_storage STATIC
    src/storage/history_store.cpp
    src/storage/bookmark_store.cpp
    src/storage/session_store.cpp
    src/storage/download_store.cpp
)
target_link_libraries(nexora_storage PUBLIC nexora_util)

#
# 3. Memory Optimization Engine Library
#
add_library(nexora_memory STATIC
    src/memory/system_memory_monitor.cpp
    src/memory/memory_pressure_controller.cpp
    src/memory/tab_priority_manager.cpp
    src/memory/tab_lifecycle_manager.cpp
    src/memory/background_resource_manager.cpp
    src/memory/memory_accounting.cpp
)
target_link_libraries(nexora_memory PUBLIC nexora_util)

#
# 4. Developer Tools Library
#
add_library(nexora_devtools STATIC
    src/devtools/task_manager.cpp
    src/devtools/memory_profiler.cpp
    src/devtools/cpu_monitor.cpp
    src/devtools/gpu_monitor.cpp
    src/devtools/devtools_manager.cpp
)
target_link_libraries(nexora_devtools PUBLIC nexora_util nexora_memory)

#
# 5. Browser Engine Library
#
add_library(nexora_engine STATIC
    src/engine/scheme_handler.cpp
    src/engine/process_manager.cpp
    src/engine/script_executor.cpp
    src/engine/cef_app_handler.cpp
    src/engine/cef_client_handler.cpp
    src/engine/browser_instance.cpp
)
target_link_libraries(nexora_engine PUBLIC nexora_util nexora_memory nexora_devtools cef_headers)

#
# 6. Browser UI Library
#
add_library(nexora_ui STATIC
    src/ui/theme_manager.cpp
    src/ui/tab_item.cpp
    src/ui/tab_group.cpp
    src/ui/tab_bar.cpp
    src/ui/address_bar.cpp
    src/ui/navigation_controls.cpp
    src/ui/bookmark_bar.cpp
    src/ui/context_menu.cpp
    src/ui/download_ui.cpp
    src/ui/browser_window.cpp
)
target_link_libraries(nexora_ui PUBLIC nexora_util nexora_storage nexora_memory nexora_devtools nexora_engine)

#
# 7. Application Core Library
#
add_library(nexora_core STATIC
    src/core/config_manager.cpp
    src/core/profile_manager.cpp
    src/core/update_manager.cpp
    src/core/window_manager.cpp
    src/core/app_lifecycle.cpp
)
target_link_libraries(nexora_core PUBLIC nexora_util nexora_storage nexora_memory nexora_devtools nexora_engine nexora_ui)

#
# Windows Main Executables
#
if(WIN32)
    add_executable(NEXORA WIN32 src/main_windows.cpp)
    target_link_libraries(NEXORA PRIVATE nexora_core)

    add_executable(NEXORA_Subprocess WIN32 src/subprocess_main.cpp)
    target_link_libraries(NEXORA_Subprocess PRIVATE nexora_engine)
else()
    add_executable(NEXORA src/main_windows.cpp)
    target_link_libraries(NEXORA PRIVATE nexora_core)

    add_executable(NEXORA_Subprocess src/subprocess_main.cpp)
    target_link_libraries(NEXORA_Subprocess PRIVATE nexora_engine)
endif()

#
# Tests & Benchmarks
#
enable_testing()
add_subdirectory(tests)
