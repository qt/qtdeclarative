# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.18)
project(qmldomloadeditwrite LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/qmldomloadeditwrite")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Qml QmlDomPrivate)

add_compile_definitions(
    QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/../../../tests/auto/qmldom/domdata"
)

qt_add_executable(qmldomloadeditwrite
    qmldomloadeditwrite.cpp
)

target_link_libraries(qmldomloadeditwrite PUBLIC
    Qt6::Core
    Qt6::Qml
    Qt6::QmlDomPrivate
)

install(TARGETS qmldomloadeditwrite
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
