# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(texteditor LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols/texteditor")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 OPTIONAL_COMPONENTS Widgets)

qt_add_executable(texteditorexample WIN32 MACOSX_BUNDLE
    documenthandler.cpp
    texteditor.cpp
)

qt_add_qml_module(texteditorexample
    URI texteditor
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        "qml/+touch/texteditor.qml"
        "qml/texteditor.qml"
    RESOURCES
        "texteditor.html"
        "+touch/texteditor.html"
        "fonts/fontello.ttf"
        "images/qt-logo.png"
)

target_link_libraries(texteditorexample PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

if(TARGET Qt6::Widgets)
    target_link_libraries(texteditorexample PUBLIC
        Qt6::Widgets
    )
endif()

if(CMAKE_CROSSCOMPILING)
    target_compile_definitions(texteditorexample PUBLIC
        QT_EXTRA_FILE_SELECTOR="touch"
    )
endif()

install(TARGETS texteditorexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
