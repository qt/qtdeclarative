# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_library(qmltc_exported_tests_module STATIC)
qt_autogen_tools_initial_setup(qmltc_exported_tests_module)

include(GenerateExportHeader)
generate_export_header(qmltc_exported_tests_module)

set(common_libraries
    Qt::QuickPrivate
)

target_link_libraries(qmltc_exported_tests_module PUBLIC ${common_libraries})

qt_policy(SET QTP0001 NEW)

qt6_add_qml_module(qmltc_exported_tests_module
    URI QmltcExportedTests
    QML_FILES
        HelloExportedWorld.qml
    DEPENDENCIES
        Qt::Quick
    ENABLE_TYPE_COMPILER
    QMLTC_EXPORT_DIRECTIVE "QMLTC_EXPORTED_TESTS_MODULE_EXPORT"
    QMLTC_EXPORT_FILE_NAME "qmltc_exported_tests_module_export.h"
)

target_include_directories(qmltc_exported_tests_module PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

qt_autogen_tools_initial_setup(qmltc_exported_tests_moduleplugin)
