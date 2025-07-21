# Add RunCMake tests using `add_RunCMake_test()`

_qt_internal_get_cmake_test_configure_options(option_list)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19.0")
    add_RunCMake_test(generate_qmlls_ini ${option_list})
endif()

add_RunCMake_test(export_namespace_in_root ${option_list})
