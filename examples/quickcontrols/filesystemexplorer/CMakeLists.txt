# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(filesystemexplorer LANGUAGES CXX)

if (ANDROID OR IOS)
    message(FATAL_ERROR "Platform is not supported")
    return()
endif ()

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
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET filesystemexplorer
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
