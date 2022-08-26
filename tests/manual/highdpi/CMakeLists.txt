# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from highdpi.pro.

#####################################################################
## qmlimageproviderplugin Generic Library:
#####################################################################

qt_internal_add_cmake_library(qmlimageproviderplugin
    MODULE
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/quick/imageprovider/ImageProvider"
    OUTPUT_DIRECTORY "ImageProvider"
    SOURCES
        imageprovider.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:highdpi.pro:<TRUE>:
# INSTALLS = "target" "qml"
# TEMPLATE = "lib"
# qml.files = "ImageProvider/qmldir"
# qml.path = "$$[QT_INSTALL_EXAMPLES]/quick/imageprovider/ImageProvider"
# target.path = "$$[QT_INSTALL_EXAMPLES]/quick/imageprovider/ImageProvider"

qt_autogen_tools_initial_setup(qmlimageproviderplugin)
