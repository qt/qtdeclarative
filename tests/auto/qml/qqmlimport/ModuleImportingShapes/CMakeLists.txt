# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_library(tst_qqmlimport_shapes_module STATIC)
qt_autogen_tools_initial_setup(tst_qqmlimport_shapes_module)

qt_add_qml_module(tst_qqmlimport_shapes_module
    URI ModuleImportingShapes
    NO_PLUGIN
    QML_FILES
        Main.qml
    IMPORTS
        QtQuick.Shapes
    RESOURCE_PREFIX "/"
)
