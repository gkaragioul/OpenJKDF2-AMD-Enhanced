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

    add_test(
        NAME windows_manifest
        COMMAND "${OPENJKDF2_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${PROJECT_SOURCE_DIR}/scripts/check-windows-manifest.ps1"
    )
    set_tests_properties(windows_manifest PROPERTIES LABELS "unit")

    add_test(
        NAME aspect_menu_contract
        COMMAND "${OPENJKDF2_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${PROJECT_SOURCE_DIR}/scripts/check-aspect-menu.ps1"
    )
    set_tests_properties(aspect_menu_contract PROPERTIES LABELS "unit")

    add_test(
        NAME video_menu_contract
        COMMAND "${OPENJKDF2_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${PROJECT_SOURCE_DIR}/scripts/check-video-menu.ps1"
    )
    set_tests_properties(video_menu_contract PROPERTIES LABELS "unit")

    add_test(
        NAME enhancement_framework_contract
        COMMAND "${OPENJKDF2_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${PROJECT_SOURCE_DIR}/scripts/check-enhancement-framework.ps1"
    )
    set_tests_properties(enhancement_framework_contract PROPERTIES LABELS "unit")

    add_test(
        NAME renderer_telemetry_contract
        COMMAND "${OPENJKDF2_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${PROJECT_SOURCE_DIR}/scripts/check-renderer-telemetry.ps1"
    )
    set_tests_properties(renderer_telemetry_contract PROPERTIES LABELS "unit")

    add_test(
        NAME diagnostics_page_contract
        COMMAND "${OPENJKDF2_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${PROJECT_SOURCE_DIR}/scripts/check-diagnostics-page.ps1"
    )
    set_tests_properties(diagnostics_page_contract PROPERTIES LABELS "unit")

    add_test(
        NAME control_preset_mapping_contract
        COMMAND "${OPENJKDF2_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${PROJECT_SOURCE_DIR}/scripts/check-control-preset-mappings.ps1"
    )
    set_tests_properties(control_preset_mapping_contract PROPERTIES LABELS "unit")

    add_test(
        NAME display_confirmation_runtime_contract
        COMMAND "${OPENJKDF2_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${PROJECT_SOURCE_DIR}/scripts/check-display-confirmation-runtime.ps1"
    )
    set_tests_properties(display_confirmation_runtime_contract PROPERTIES LABELS "unit")

    add_test(
        NAME release_documentation_contract
        COMMAND "${OPENJKDF2_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${PROJECT_SOURCE_DIR}/scripts/check-release-documentation.ps1"
    )
    set_tests_properties(release_documentation_contract PROPERTIES LABELS "unit")

    add_test(
        NAME mouse_latency_telemetry_contract
        COMMAND "${OPENJKDF2_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${PROJECT_SOURCE_DIR}/scripts/check-mouse-latency-telemetry.ps1"
    )
    set_tests_properties(mouse_latency_telemetry_contract PROPERTIES LABELS "unit")

    add_test(
        NAME enhancement_runtime_telemetry_contract
        COMMAND "${OPENJKDF2_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${PROJECT_SOURCE_DIR}/scripts/check-enhancement-runtime-telemetry.ps1"
    )
    set_tests_properties(enhancement_runtime_telemetry_contract PROPERTIES LABELS "unit")

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
        test_path_overlay
        "${PROJECT_SOURCE_DIR}/src/Tests/test_path_overlay.c"
        "${PROJECT_SOURCE_DIR}/src/General/PathOverlay.c"
    )
    openjkdf2_add_unit_test(
        test_storage_paths
        "${PROJECT_SOURCE_DIR}/src/Tests/test_storage_paths.c"
        "${PROJECT_SOURCE_DIR}/src/General/StoragePaths.c"
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
        test_renderer_diagnostics
        "${PROJECT_SOURCE_DIR}/src/Tests/test_renderer_diagnostics.c"
        "${PROJECT_SOURCE_DIR}/src/General/RendererDiagnostics.c"
    )
    openjkdf2_add_unit_test(
        test_display_mode
        "${PROJECT_SOURCE_DIR}/src/Tests/test_display_mode.c"
        "${PROJECT_SOURCE_DIR}/src/General/DisplayMode.c"
    )
    openjkdf2_add_unit_test(
        test_display_transaction
        "${PROJECT_SOURCE_DIR}/src/Tests/test_display_transaction.c"
        "${PROJECT_SOURCE_DIR}/src/General/DisplayTransaction.c"
    )
    openjkdf2_add_unit_test(
        test_display_selection
        "${PROJECT_SOURCE_DIR}/src/Tests/test_display_selection.c"
        "${PROJECT_SOURCE_DIR}/src/General/DisplaySelection.c"
    )
    openjkdf2_add_unit_test(
        test_resolution_layout
        "${PROJECT_SOURCE_DIR}/src/Tests/test_resolution_layout.c"
        "${PROJECT_SOURCE_DIR}/src/General/ResolutionLayout.c"
    )
    openjkdf2_add_unit_test(
        test_aspect_policy
        "${PROJECT_SOURCE_DIR}/src/Tests/test_aspect_policy.c"
        "${PROJECT_SOURCE_DIR}/src/General/AspectPolicy.c"
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
    openjkdf2_add_unit_test(
        test_fixed_step
        "${PROJECT_SOURCE_DIR}/src/Tests/test_fixed_step.c"
        "${PROJECT_SOURCE_DIR}/src/General/FixedStep.c"
    )
    openjkdf2_add_unit_test(
        test_control_preset
        "${PROJECT_SOURCE_DIR}/src/Tests/test_control_preset.c"
        "${PROJECT_SOURCE_DIR}/src/General/ControlPreset.c"
    )
    openjkdf2_add_unit_test(
        test_mouse_smoothing
        "${PROJECT_SOURCE_DIR}/src/Tests/test_mouse_smoothing.c"
        "${PROJECT_SOURCE_DIR}/src/General/MouseSmoothing.c"
    )
    openjkdf2_add_unit_test(
        test_frame_telemetry
        "${PROJECT_SOURCE_DIR}/src/Tests/test_frame_telemetry.c"
        "${PROJECT_SOURCE_DIR}/src/General/FrameTelemetry.c"
    )
    openjkdf2_add_unit_test(
        test_runtime_probe
        "${PROJECT_SOURCE_DIR}/src/Tests/test_runtime_probe.c"
        "${PROJECT_SOURCE_DIR}/src/General/RuntimeProbe.c"
    )
    openjkdf2_add_unit_test(
        test_timing_domains_observer
        "${PROJECT_SOURCE_DIR}/src/Tests/test_timing_domains_observer.c"
        "${PROJECT_SOURCE_DIR}/src/General/TimingDomainsObserver.c"
    )
    openjkdf2_add_unit_test(
        test_save_load_probe
        "${PROJECT_SOURCE_DIR}/src/Tests/test_save_load_probe.c"
        "${PROJECT_SOURCE_DIR}/src/General/SaveLoadProbe.c"
    )
    openjkdf2_add_unit_test(
        test_quality_preset
        "${PROJECT_SOURCE_DIR}/src/Tests/test_quality_preset.c"
        "${PROJECT_SOURCE_DIR}/src/General/QualityPreset.c"
    )
    openjkdf2_add_unit_test(
        test_video_defaults
        "${PROJECT_SOURCE_DIR}/src/Tests/test_video_defaults.c"
        "${PROJECT_SOURCE_DIR}/src/General/VideoDefaults.c"
    )
    openjkdf2_add_unit_test(
        test_config_recovery
        "${PROJECT_SOURCE_DIR}/src/Tests/test_config_recovery.c"
        "${PROJECT_SOURCE_DIR}/src/General/ConfigRecovery.c"
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
