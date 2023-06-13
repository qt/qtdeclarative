# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_cmake_library(qmlhighdpiimageproviderplugin
    MODULE
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/quick/imageprovider/HighDPIImageProvider"
    OUTPUT_DIRECTORY "HighDPIImageProvider"
    SOURCES
        imageprovider.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

qt_autogen_tools_initial_setup(qmlhighdpiimageproviderplugin)
