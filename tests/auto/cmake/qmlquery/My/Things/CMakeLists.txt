# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set_source_files_properties(Two.qml PROPERTIES
    QT_RESOURCE_ALIAS subdir/Two.qml
)
set_source_files_properties(Two.txt PROPERTIES
    QT_RESOURCE_ALIAS subdir/Two.txt
)

qt_add_qml_module(MyThings
    URI My.Things
    VERSION 1.3
    RESOURCE_PREFIX mine.example.com/imports
    QML_FILES
        One.qml
        Two.qml
    RESOURCES
        One.txt
        Two.txt
)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
    # PREFIX deliberately has no leading "/". We want to test that we prepend
    # "/" when it is missing and that this prepended character is also present
    # in our query result.
    qt_target_qml_sources(MyThings
        PREFIX somewhere.else.com/nested/rather/deep
        QML_FILES Three.qml
        RESOURCES Three.txt
    )
endif()

qt_query_qml_module(MyThings
    URI uri
    VERSION version
    PLUGIN_TARGET plugin_target
    MODULE_RESOURCE_PATH module_resource_path
    TARGET_PATH target_path
    QMLDIR qmldir
    TYPEINFO typeinfo
    QML_FILES qml_files
    QML_FILES_DEPLOY_PATHS qml_files_deploy_paths
    QML_FILES_PREFIX_OVERRIDES qml_files_prefix_overrides
    RESOURCES resources
    RESOURCES_DEPLOY_PATHS resources_deploy_paths
    RESOURCES_PREFIX_OVERRIDES resources_prefix_overrides
)

function(verify_result keyword expected actual)
    if(NOT "${actual}" STREQUAL "${expected}")
        message(SEND_ERROR
            "  Expected ${keyword}: ${expected}\n"
            "  Actual   ${keyword}: ${actual}"
        )
    endif()
endfunction()

verify_result(URI                  "My.Things" "${uri}")
verify_result(VERSION              "1.3" "${version}")
verify_result(PLUGIN_TARGET        "MyThingsplugin" "${plugin_target}")
verify_result(MODULE_RESOURCE_PATH "/mine.example.com/imports/My/Things" "${module_resource_path}")
verify_result(TARGET_PATH          "My/Things" "${target_path}")
verify_result(QMLDIR               "${CMAKE_CURRENT_BINARY_DIR}/qmldir" "${qmldir}")
verify_result(TYPEINFO             "${CMAKE_CURRENT_BINARY_DIR}/MyThings.qmltypes" "${typeinfo}")

# QML_FILES
set(expected_files "${CMAKE_CURRENT_SOURCE_DIR}/One.qml" "${CMAKE_CURRENT_SOURCE_DIR}/Two.qml")
set(expected_deploy_paths "One.qml" "subdir/Two.qml")
set(expected_prefix_overrides "" "")
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
    list(APPEND expected_files "${CMAKE_CURRENT_SOURCE_DIR}/Three.qml")
    list(APPEND expected_deploy_paths "")   # Empty when prefix is overridden
    list(APPEND expected_prefix_overrides "/somewhere.else.com/nested/rather/deep")
endif()
verify_result(QML_FILES "${expected_files}" "${qml_files}")
verify_result(QML_FILES_DEPLOY_PATHS "${expected_deploy_paths}" "${qml_files_deploy_paths}")
verify_result(QML_FILES_PREFIX_OVERRIDES
              "${expected_prefix_overrides}" "${qml_files_prefix_overrides}")

# RESOURCES
set(expected_files "${CMAKE_CURRENT_SOURCE_DIR}/One.txt" "${CMAKE_CURRENT_SOURCE_DIR}/Two.txt")
set(expected_deploy_paths "One.txt" "subdir/Two.txt")
set(expected_prefix_overrides "" "")
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
    list(APPEND expected_files "${CMAKE_CURRENT_SOURCE_DIR}/Three.txt")
    list(APPEND expected_deploy_paths "")   # Empty when prefix is overridden
    list(APPEND expected_prefix_overrides "/somewhere.else.com/nested/rather/deep")
endif()
verify_result(RESOURCES "${expected_files}" "${resources}")
verify_result(RESOURCES_DEPLOY_PATHS "${expected_deploy_paths}" "${resources_deploy_paths}")
verify_result(RESOURCES_PREFIX_OVERRIDES
              "${expected_prefix_overrides}" "${resources_prefix_overrides}")
