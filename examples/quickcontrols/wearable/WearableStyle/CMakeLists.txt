# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(wearablestyle LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols/wearable/WearableStyle")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

set_source_files_properties(UIStyle.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_policy(SET QTP0001 NEW)
qt_add_qml_module(wearablestyle
    URI WearableStyle
    PLUGIN_TARGET wearablestyle
    QML_FILES
        PageIndicator.qml
        Slider.qml
        Switch.qml
        UIStyle.qml
)

target_link_libraries(wearablestyle PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

if(UNIX AND NOT APPLE AND CMAKE_CROSSCOMPILING)
    find_package(Qt6 REQUIRED COMPONENTS QuickTemplates2)

    # Work around QTBUG-86533
    target_link_libraries(wearablestyle PRIVATE Qt6::QuickTemplates2)
endif()

install(TARGETS wearablestyle
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmldir
    DESTINATION "${INSTALL_EXAMPLEDIR}"
)
