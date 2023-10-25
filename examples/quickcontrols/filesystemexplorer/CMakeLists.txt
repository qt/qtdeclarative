# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(filesystemexplorer LANGUAGES CXX)

if (NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif ()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols/filesystemexplorer")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Svg)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(filesystemexplorer
    main.cpp
)

set_target_properties(filesystemexplorer
    PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
)

set_source_files_properties(qml/Colors.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(filesystemexplorer
    URI FileSystemModule
    VERSION 1.0
    QML_FILES
        "Main.qml"
        "qml/About.qml"
        "qml/Colors.qml"
        "qml/Editor.qml"
        "qml/FileSystemView.qml"
        "qml/MyMenu.qml"
        "qml/MyMenuBar.qml"
        "qml/Sidebar.qml"
        "qml/ResizeButton.qml"
        "qml/WindowDragHandler.qml"
    RESOURCES
        "icons/folder_closed.svg"
        "icons/folder_open.svg"
        "icons/generic_file.svg"
        "icons/globe.svg"
        "icons/info_sign.svg"
        "icons/light_bulb.svg"
        "icons/read.svg"
        "icons/resize.svg"
        "icons/qt_logo.svg"
        "icons/app_icon.svg"
    SOURCES
        filesystemmodel.cpp
        filesystemmodel.h
        linenumbermodel.cpp
        linenumbermodel.h
)

target_link_libraries(filesystemexplorer
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Svg
)

install(TARGETS filesystemexplorer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
