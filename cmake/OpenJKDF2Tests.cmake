if(NOT DEFINED BUILD_TESTING)
    message(FATAL_ERROR "CTest must define BUILD_TESTING before OpenJKDF2 tests are configured")
endif()

add_custom_target(openjkdf2-unit-tests)

function(openjkdf2_add_unit_test name source)
    add_executable(${name} ${source} ${ARGN})
    target_compile_features(${name} PRIVATE c_std_11)
    target_include_directories(${name} PRIVATE "${PROJECT_SOURCE_DIR}/src")
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES LABELS "unit")
    add_dependencies(openjkdf2-unit-tests ${name})
endfunction()

if(BUILD_TESTING)
    find_program(OPENJKDF2_POWERSHELL NAMES pwsh powershell REQUIRED)
    add_test(
        NAME c11_portability
        COMMAND "${OPENJKDF2_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${PROJECT_SOURCE_DIR}/scripts/check-c11-portability.ps1"
    )
    set_tests_properties(c11_portability PROPERTIES LABELS "unit")

    openjkdf2_add_unit_test(
        test_diagnostic_log
        "${PROJECT_SOURCE_DIR}/src/Tests/test_diagnostic_log.c"
        "${PROJECT_SOURCE_DIR}/src/General/DiagnosticLog.c"
    )
    openjkdf2_add_unit_test(
        test_startup_options
        "${PROJECT_SOURCE_DIR}/src/Tests/test_startup_options.c"
        "${PROJECT_SOURCE_DIR}/src/General/StartupOptions.c"
    )
endif()
