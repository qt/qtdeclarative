# Add RunCMake tests using `add_RunCMake_test()`

_qt_internal_get_cmake_test_configure_options(config_flags)

if(NOT CMAKE_CROSSCOMPILING)
    qt_internal_add_RunCMake_test(qt_target_qml_sources ${config_flags})
    qt_internal_add_RunCMake_test(qmlcachegen_path_sanitization ${config_flags})
    if(QT_FEATURE_qmlcontextpropertydump)
        qt_internal_add_RunCMake_test(generate_context_property_dump ${config_flags})
    endif()
    if(NOT QT_BUILD_MINIMAL_STATIC_TESTS)
        qt_internal_add_RunCMake_test(aot_lookup_validation ${config_flags} "${QT6_INSTALL_PREFIX}" "${QT6_INSTALL_BINS}")
    endif()
    if(QT6_IS_SHARED_LIBS_BUILD)
        qt_internal_add_RunCMake_test(qt_generate_deploy_qml_app_script ${config_flags})
    endif()
    # host qmlpreview cannot interact with binaries on the target
    # should this ever change, we still could not run the test without
    # qemu/an enumlator to actually run the target
    qt_internal_add_RunCMake_test(qt_add_qml_preview ${config_flags})
endif()
qt_internal_add_RunCMake_test(export_namespace_in_root ${config_flags})

qt_internal_add_RunCMake_test(generate_qmlls_ini ${config_flags})
qt_internal_add_RunCMake_test(qmllint_targets ${config_flags})
