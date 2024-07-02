# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(texteditor LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(texteditorexample WIN32 MACOSX_BUNDLE
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

if(CMAKE_CROSSCOMPILING)
    target_compile_definitions(texteditorexample PUBLIC
        QT_EXTRA_FILE_SELECTOR="touch"
    )
endif()

install(TARGETS texteditorexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET texteditorexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
