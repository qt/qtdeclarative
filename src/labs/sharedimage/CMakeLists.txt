# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(LabsSharedImage
    URI "Qt.labs.sharedimage"
    VERSION "${PROJECT_VERSION}"
    __QT_INTERNAL_STATIC_MODULE
    PLUGIN_TARGET sharedimageplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    CLASS_NAME QtQuickSharedImagePlugin
    SOURCES
        qsharedimageloader.cpp qsharedimageloader_p.h
        qsharedimageprovider.cpp qsharedimageprovider_p.h
        qtlabssharedimageglobal_p.h
    DEFINES
        QT_BUILD_LABSSHAREDIMAGE_LIB
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QuickPrivate
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

# We need to do additional initialization, so we have to provide our own
# plugin class rather than using the generated one
qt_internal_extend_target(sharedimageplugin
    SOURCES   qsharedimageplugin.cpp
    LIBRARIES Qt::LabsSharedImagePrivate
)
