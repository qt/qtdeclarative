# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.20)

project(MyApplication VERSION 0.1 LANGUAGES CXX)

qt_standard_project_setup(REQUIRES 6.6)

find_package(Qt6 COMPONENTS REQUIRED Quick QuickControls2)

#! [project-cmake]
add_subdirectory(MyModule)
add_subdirectory(tests)

qt_add_executable(MyApplication
    src/main.cpp
)

qt_add_qml_module(MyApplication
    URI MyApplication
    QML_FILES main.qml
)
#! [project-cmake]

set_target_properties(MyApplication PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.demoapp.com
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(MyApplication
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(MyApplication
    PRIVATE
       Qt6::Quick
       Qt6::QuickControls2
       MyModule
)

install(TARGETS MyApplication
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
