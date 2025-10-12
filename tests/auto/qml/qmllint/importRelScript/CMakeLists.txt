# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_policy(SET QTP0001 NEW)

set(CMAKE_AUTOMOC ON)

qt_add_qml_module(tst_qmllint_import_rel_script
    URI ImportRelScript
    QML_FILES
        Main.qml
        script.js
    PLUGIN_TARGET
        tst_qmllint_import_rel_script
)

qt_autogen_tools_initial_setup(tst_qmllint_import_rel_script)
add_dependencies(tst_qmllint_import_rel_script Qt::qmllint)
