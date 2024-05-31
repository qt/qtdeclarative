# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(menus VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appmenus
    main.cpp
)

qt_add_qml_module(appmenus
    URI Menus
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES
        cppsettings.cpp
        cppsettings.h
        main.cpp
    RESOURCES
        icons/warning.png
        icons/warning@2x.png
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appmenus PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appmenus
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appmenus
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
)
