include(QtRunCMake)

include(${_Qt6CTestMacros})
# Stub function to make `_qt_internal_get_cmake_test_configure_options` work
function(_qt_internal_get_build_vars_for_external_projects)
endfunction()

_qt_internal_get_cmake_test_configure_options(config_flags)
list(APPEND config_flags "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")

function(run_cmake_and_build case)
    # Set common build directory for configure and build
    # Make sure the build dir doesn't contain the ${case} because
    # its too long on Windows then.
    set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/build")
    run_cmake_with_options(${case} ${config_flags})
    # Do not remove the current RunCMake_TEST_BINARY_DIR
    set(RunCMake_TEST_NO_CLEAN 1)
    run_cmake_command(${case}-build ${CMAKE_COMMAND} --build .)
endfunction()

# Verifies that the qmlcachegen output file names produced for the module's
# QML files contain underscores in place of the characters that the
# sanitization regex must rewrite (".." traversals, slashes, and spaces).
# Only infix matching is performed, not full filename matching, so that the
# assertions stay robust against changes to the surrounding name parts.
function(check_qmlcache_generated_files case module_build_dir)
    set(qmlcache_dir "${module_build_dir}/.rcc/qmlcache")
    if(NOT IS_DIRECTORY "${qmlcache_dir}")
        message(FATAL_ERROR
            "Test failed: qmlcache directory not found: ${qmlcache_dir}")
    endif()

    file(GLOB generated_files RELATIVE "${qmlcache_dir}" "${qmlcache_dir}/*.cpp")
    list(JOIN generated_files "\n" file_list)

    # Each entry is a substring that the sanitized filename for one of the
    # module's QML files must contain.
    # The expected underscore counts come from:
    #   <target>_<compiled_file>.cpp
    # where `compiled_file` has each "../" replaced with "_", spaces and
    # slashes collapsed to "_", and the ".qml" extension's "." replaced with
    # "_". So one "_" comes from the "<target>_" prefix and each "../" contributes
    # an additional leading underscore.
    set(expected_infixes
        "_Main_qml"                  # Main.qml (prefix)
        "__ParentFoo_qml"            # ../ParentFoo.qml (prefix + one '..')
        "___GrandFoo_qml"            # ../../GrandFoo.qml (prefix + two '..')
        "__sibling_SiblingFoo_qml"   # ../sibling/SiblingFoo.qml
        "__side_dir_SpacedFoo_qml"   # ../side dir/SpacedFoo.qml
        "_BuildFoo_qml"              # ${CMAKE_CURRENT_BINARY_DIR}/BuildFoo.qml (binary dir base)
        "_OuterBuildFoo_qml"         # ${CMAKE_CURRENT_BINARY_DIR}/../OuterBuildFoo.qml
                                     # (variable number of leading '_' since the count of
                                     # '..'s in the relative path depends on the build vs
                                     # source layout)
    )

    foreach(infix IN LISTS expected_infixes)
        string(FIND "${file_list}" "${infix}" find_result)
        if(find_result EQUAL -1)
            message(FATAL_ERROR
                "Test failed: no generated qmlcache file name contains '${infix}'."
                " Files in ${qmlcache_dir}:\n${file_list}")
        endif()
        message(STATUS "${case}-check-infix${infix} - PASSED")
    endforeach()

    # The sanitized filenames must not still contain ".." or a space.
    foreach(generated_file IN LISTS generated_files)
        if(generated_file MATCHES "\\.\\." OR generated_file MATCHES " ")
            message(FATAL_ERROR
                "Test failed: sanitized filename still contains '..' or a space:"
                " ${generated_file}")
        endif()
    endforeach()
    message(STATUS "${case}-check-no-dotdot-or-space - PASSED")
endfunction()

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19"
    AND RunCMake_GENERATOR MATCHES "^Ninja"
)
    run_cmake_and_build(qmlcachegen_path_sanitization)
    string(CONCAT module_build_dir "${RunCMake_BINARY_DIR}/build/parent/Potato")
    check_qmlcache_generated_files(qmlcachegen_path_sanitization "${module_build_dir}")
endif()
