# Add RunCMake tests using `add_RunCMake_test()`

_qt_internal_get_cmake_test_configure_options(config_flags)

if(NOT CMAKE_CROSSCOMPILING)
    qt_internal_add_RunCMake_test(qt_target_qml_sources ${config_flags})
endif()
qt_internal_add_RunCMake_test(export_namespace_in_root ${config_flags})

qt_internal_add_RunCMake_test(generate_qmlls_ini ${config_flags})
