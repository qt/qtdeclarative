# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(ObjectRegistryTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Core QmlDesignSupport)

qt_standard_project_setup(REQUIRES 6.12)

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/objectregistry")

qt_add_executable(appObjectRegistryTest
    main.cpp
)

qt_add_qml_module(appObjectRegistryTest
    URI ObjectRegistryTest
    QML_FILES
        Main.qml
        BusinessLogic.qml
        Ui.qml
    SOURCES
        cppbusinesslogic.h
        cppbusinesslogic.cpp
)

target_link_libraries(appObjectRegistryTest PRIVATE
    Qt6::Quick
    Qt6::QmlDesignSupport
    Qt6::Core
)

install(TARGETS appObjectRegistryTest
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
