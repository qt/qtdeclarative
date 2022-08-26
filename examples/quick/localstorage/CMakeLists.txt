# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(localstorage LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/localstorage")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(localstorageexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

qt_add_qml_module(localstorageexample
    URI localstorage
    VERSION 1.0
    AUTO_RESOURCE_PREFIX
    QML_FILES
        "Database.js"
        "Header.qml"
        "MyDelegate.qml"
        "MyModel.qml"
        "localstorage.qml"
    )

target_link_libraries(localstorageexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

install(TARGETS localstorageexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
