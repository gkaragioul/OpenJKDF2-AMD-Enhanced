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
    openjkdf2_add_unit_test(
        test_shader_stage
        "${PROJECT_SOURCE_DIR}/src/Tests/test_shader_stage.c"
        "${PROJECT_SOURCE_DIR}/src/Platform/GL/ShaderCompile.c"
    )
    target_compile_definitions(test_shader_stage PRIVATE OPENJKDF2_SHADER_PURE_TEST)
    openjkdf2_add_unit_test(
        test_diagnostic_report
        "${PROJECT_SOURCE_DIR}/src/Tests/test_diagnostic_report.c"
        "${PROJECT_SOURCE_DIR}/src/General/DiagnosticReport.c"
    )
    openjkdf2_add_unit_test(
        test_display_mode
        "${PROJECT_SOURCE_DIR}/src/Tests/test_display_mode.c"
        "${PROJECT_SOURCE_DIR}/src/General/DisplayMode.c"
    )
    openjkdf2_add_unit_test(
        test_resolution_layout
        "${PROJECT_SOURCE_DIR}/src/Tests/test_resolution_layout.c"
        "${PROJECT_SOURCE_DIR}/src/General/ResolutionLayout.c"
    )
    openjkdf2_add_unit_test(
        test_frame_rate
        "${PROJECT_SOURCE_DIR}/src/Tests/test_frame_rate.c"
        "${PROJECT_SOURCE_DIR}/src/General/FrameRate.c"
    )
    openjkdf2_add_unit_test(
        test_presentation_mode
        "${PROJECT_SOURCE_DIR}/src/Tests/test_presentation_mode.c"
        "${PROJECT_SOURCE_DIR}/src/General/PresentationMode.c"
    )

    if(TARGET_USE_SDL2 AND TARGET_USE_OPENGL AND NOT TARGET_ANDROID AND NOT TARGET_WASM)
        add_executable(
            openjkdf2-renderer-smoke
            "${PROJECT_SOURCE_DIR}/src/Tools/renderer_smoke_main.c"
            "${PROJECT_SOURCE_DIR}/src/General/DiagnosticLog.c"
            "${PROJECT_SOURCE_DIR}/src/General/DiagnosticReport.c"
            "${PROJECT_SOURCE_DIR}/src/Platform/GL/ShaderCompile.c"
        )
        target_compile_features(openjkdf2-renderer-smoke PRIVATE c_std_11)
        target_include_directories(openjkdf2-renderer-smoke PRIVATE "${PROJECT_SOURCE_DIR}/src")
        target_link_libraries(openjkdf2-renderer-smoke PRIVATE ${SDL2_COMMON_LIBS} GLEW::glew_s)
        if(WIN32)
            target_link_libraries(openjkdf2-renderer-smoke PRIVATE
                opengl32 version imm32 setupapi cfgmgr32 winmm ole32 oleaut32 shell32 user32
            )
        endif()
        add_test(NAME renderer_smoke COMMAND openjkdf2-renderer-smoke)
        set_tests_properties(renderer_smoke PROPERTIES LABELS "renderer-smoke" DISABLED TRUE)
    endif()
endif()
