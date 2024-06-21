# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_standard_project_setup(REQUIRES 6.5 I18N_TRANSLATED_LANGUAGES fr)

qt_internal_add_executable(i18nLoadFromModuleQmlType
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
)

qt_add_qml_module(i18nLoadFromModuleQmlType
    URI TranslatedQml
    QML_FILES Main.qml
)

qt_internal_extend_target(i18nLoadFromModuleQmlType
    ENABLE_AUTOGEN_TOOLS
        uic
)

qt_add_resources(i18nLoadFromModuleQmlType "loadFromModuleQmlTypeQmFile"
    PREFIX
        /qt/qml/TranslatedQml/
    FILES
        i18n/qml_fr.qm
)
