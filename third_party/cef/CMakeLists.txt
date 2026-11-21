# CEF integration CMake configuration

add_library(cef_headers INTERFACE)
target_include_directories(cef_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
