include(QtRunCMake)

include(${_Qt6CTestMacros})
# Stub function to make `_qt_internal_get_cmake_test_configure_options` work
function(_qt_internal_get_build_vars_for_external_projects)
endfunction()

_qt_internal_get_cmake_test_configure_options(config_flags)
list(APPEND config_flags "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")

set(RunCMake_TEST_OUTPUT_MERGE TRUE)

# Opting in to QTP0007 must not warn about it.
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/deploy-tool-options-policy-set-build")
set(RunCMake_TEST_NOT_EXPECT_stdout "QTP0007")
run_cmake_with_options(deploy-tool-options-policy-set ${config_flags})
unset(RunCMake_TEST_NOT_EXPECT_stdout)
