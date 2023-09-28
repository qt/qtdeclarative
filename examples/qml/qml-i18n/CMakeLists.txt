# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qml-i18n LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/qmli18n")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml LinguistTools)

qt_standard_project_setup(I18N_LANGUAGES en en_AU fr)

qt_add_executable(qmli18n WIN32 MACOSX_BUNDLE main.cpp)

target_link_libraries(qmli18n PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(qmli18n
    URI i18n
    QML_FILES
        qml-i18n.qml
    NO_RESOURCE_TARGET_PATH
)

qt_add_translations(qmli18n
    TS_FILE_BASE qml
    TS_FILE_DIR i18n
)

install(TARGETS qmli18n
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
