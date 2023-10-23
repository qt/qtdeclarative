# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qml-i18n LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/qmli18n")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml LinguistTools)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(qmli18n WIN32 MACOSX_BUNDLE main.cpp)

target_link_libraries(qmli18n PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(qmli18n
    URI translated
    QML_FILES
        qml-i18n.qml
)

qt_add_translations(qmli18n
    RESOURCE_PREFIX /qt/qml/translated/i18n
    TS_FILES
        i18n/base.ts
        i18n/qml_en.ts
        i18n/qml_en_AU.ts
        i18n/qml_fr.ts
)

install(TARGETS qmli18n
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
