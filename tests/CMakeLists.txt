add_executable(test_runner
    test_main.cpp
    test_application_core.cpp
    test_browser_ui.cpp
    test_memory_engine.cpp
    test_developer_tools.cpp
    test_storage.cpp
    test_benchmarks.cpp
    test_cef_integration.cpp
    test_stability_and_security.cpp
)

target_link_libraries(test_runner PRIVATE
    nexora_core
    nexora_ui
    nexora_engine
    nexora_devtools
    nexora_memory
    nexora_storage
    nexora_util
)

add_test(NAME NEXORA_AllTests COMMAND test_runner)
