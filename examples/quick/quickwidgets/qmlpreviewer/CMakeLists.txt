# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(QMLPreviewerExample VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickWidgets)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
    main.cpp
    models/errorlistmodel.h models/errorlistmodel.cpp
    states/statecontroller.h states/statecontroller.cpp
    utility/syntaxhighlighter.h utility/syntaxhighlighter.cpp
    widgets/codeeditor.h widgets/codeeditor.cpp
    widgets/editorwidget.h widgets/editorwidget.cpp
    widgets/linenumberarea.h widgets/linenumberarea.cpp
    widgets/mainwindow.h widgets/mainwindow.cpp
    widgets/patheditwidget.h widgets/patheditwidget.cpp
    widgets/previewwidget.h widgets/previewwidget.cpp
)

include(resources.cmake)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickWidgets
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
