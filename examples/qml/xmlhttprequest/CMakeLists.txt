# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(xmlhttprequest LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/xmlhttprequest")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(xmlhttprequestexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

qt_add_qml_module(xmlhttprequestexample
    URI xmlhttprequest
    AUTO_RESOURCE_PREFIX
    QML_FILES
        "methods.js"
        "xmlhttprequest.qml"
    RESOURCES
        data.xml
)

target_link_libraries(xmlhttprequestexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

install(TARGETS xmlhttprequestexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
