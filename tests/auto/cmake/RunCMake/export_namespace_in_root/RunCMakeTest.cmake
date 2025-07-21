include(RunCMake)

include(${_Qt6CTestMacros})
# Stub function to make `_qt_internal_get_cmake_test_configure_options` work
function(_qt_internal_get_build_vars_for_external_projects)
endfunction()

_qt_internal_get_cmake_test_configure_options(config_flags)
list(APPEND config_flags "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")

function(run_cmake_and_build case)
    # Set common build directory for configure and build
    set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${case}-build)
    run_cmake_with_options(${case} ${config_flags})
    # Do not remove the current RunCMake_TEST_BINARY_DIR
    set(RunCMake_TEST_NO_CLEAN 1)
    run_cmake_command(${case}-build ${CMAKE_COMMAND} --build .)
endfunction()

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19"
    AND RunCMake_GENERATOR MATCHES "^Ninja"
)
    run_cmake_and_build(export_namespace_in_root)
endif()
