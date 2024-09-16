# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(tst_qmltestexample LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qmltest/qmltest")

find_package(Qt6 REQUIRED COMPONENTS Core Gui QuickTest)

qt_add_executable(tst_qmltestexample
    tst_qmltest.cpp
)

set_target_properties(tst_qmltestexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(tst_qmltestexample PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::QuickTest
)

if(MACOS AND QT_BUILDING_QT)
    set_target_properties(tst_qmltestexample PROPERTIES
        MACOSX_BUNDLE FALSE
    )
endif()

set(tst_files tst_basic.qml tst_item.qml)

install(FILES ${tst_files}
        DESTINATION "${INSTALL_EXAMPLEDIR}")

install(TARGETS tst_qmltestexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
