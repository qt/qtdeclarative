# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_policy(SET QTP0001 NEW)

set(CMAKE_AUTOMOC ON)

qt_add_qml_module(tst_qmllint_import_rel_script
    URI importRelScript
    QML_FILES
        Main.qml
        script.js
    PLUGIN_TARGET
        tst_qmllint_import_rel_script
)

qt_autogen_tools(tst_qmllint_import_rel_script ENABLE_AUTOGEN_TOOLS moc)
if(TARGET tst_qmllint_import_rel_script_init)
    qt_autogen_tools(tst_qmllint_import_rel_script_init ENABLE_AUTOGEN_TOOLS moc)
endif()
add_dependencies(tst_qmllint_import_rel_script Qt::qmllint)
