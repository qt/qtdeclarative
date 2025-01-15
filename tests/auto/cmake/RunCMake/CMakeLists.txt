# Add RunCMake tests using `add_RunCMake_test()`

_qt_internal_get_cmake_test_configure_options(config_flags)

if(NOT CMAKE_CROSSCOMPILING)
    add_RunCMake_test(qt_target_qml_sources ${config_flags} "-D_Qt6CTestMacros=${_Qt6CTestMacros}")
endif()

_qt_internal_get_cmake_test_configure_options(option_list)
add_RunCMake_test(generate_qmlls_ini ${option_list})
