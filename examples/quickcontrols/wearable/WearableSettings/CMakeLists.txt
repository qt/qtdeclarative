# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(wearablesettings LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

set_source_files_properties(
    WearableSettings.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_policy(SET QTP0001 NEW)
qt_add_qml_module(wearablesettings
    URI WearableSettings
    PLUGIN_TARGET wearablesettings
    QML_FILES
        WearableSettings.qml
)

target_link_libraries(wearablesettings PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

if(UNIX AND NOT APPLE AND CMAKE_CROSSCOMPILING)
    find_package(Qt6 REQUIRED COMPONENTS QuickTemplates2)

    # Work around QTBUG-86533
    target_link_libraries(wearablesettings PRIVATE Qt6::QuickTemplates2)
endif()

install(TARGETS wearablesettings
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/WearableSettings"
    LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}/WearableSettings"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmldir
    DESTINATION "${CMAKE_INSTALL_BINDIR}/WearableSettings"
)
