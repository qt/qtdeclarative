# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_javascript_files)

find_package(Qt6 REQUIRED COMPONENTS Qml)

qt_standard_project_setup(REQUIRES 6.8)

set_source_files_properties(Excluded.js PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)

set_source_files_properties(Included.js PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY FALSE
)

qt_add_qml_module(javascript_files
    URI test_javascript_files
    QML_FILES
        Bad.js
        Good.js
        Good.mjs
        Excluded.js
        Included.js
        lower.js
)
