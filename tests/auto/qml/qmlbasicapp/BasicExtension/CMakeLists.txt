# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_library(additional_qml_module STATIC)
qt_autogen_tools_initial_setup(additional_qml_module)

qt_add_plugin(additional_qml_module_plugin STATIC PLUGIN_TYPE qml_plugin CLASS_NAME BasicExtension)
qt_autogen_tools_initial_setup(additional_qml_module_plugin)

qt6_add_qml_module(additional_qml_module
    PLUGIN_TARGET additional_qml_module_plugin
    CLASS_NAME "BasicExtension"
    URI "BasicExtension"
    RESOURCE_PREFIX "/"
    QML_FILES
        Extension.qml
        More.ui.qml
        Less.js
        ESModule.mjs
        lowerCase.js
        lowerCaseModule.mjs
)
