# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(cpp_sources
)

set(qml_sources
    Type.qml
)

set(js_sources
)

set(common_libraries
    Qt::Core
    Qt::QmlPrivate
    Qt::QuickPrivate
    Qt::TestPrivate
    Qt::Gui # QColor, QMatrix4x4, ...
)

qt_add_library(qmltc_namespace_test_module STATIC)
qt_autogen_tools_initial_setup(qmltc_namespace_test_module)

target_link_libraries(qmltc_namespace_test_module PUBLIC ${common_libraries})

qt_policy(SET QTP0001 NEW)

qt6_add_qml_module(qmltc_namespace_test_module
    URI NamespaceTest.Subfolder
    SOURCES
        ${cpp_sources}
    QML_FILES
        ${qml_sources}
        ${js_sources}
    DEPENDENCIES
        QtQuick
    ENABLE_TYPE_COMPILER
)

qt_autogen_tools_initial_setup(qmltc_namespace_test_moduleplugin)
