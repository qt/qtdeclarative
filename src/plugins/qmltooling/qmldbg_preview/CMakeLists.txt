# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmldbg_preview.pro.

#####################################################################
## QQmlPreviewServiceFactory Plugin:
#####################################################################

qt_internal_add_plugin(QQmlPreviewServiceFactoryPlugin
    OUTPUT_NAME qmldbg_preview
    CLASS_NAME QQmlPreviewServiceFactory
    PLUGIN_TYPE qmltooling
    SOURCES
        qqmlpreviewblacklist.cpp qqmlpreviewblacklist.h
        qqmlpreviewfileengine.cpp qqmlpreviewfileengine.h
        qqmlpreviewfileloader.cpp qqmlpreviewfileloader.h
        qqmlpreviewhandler.cpp qqmlpreviewhandler.h
        qqmlpreviewposition.cpp qqmlpreviewposition.h
        qqmlpreviewservice.cpp qqmlpreviewservice.h
        qqmlpreviewservicefactory.cpp qqmlpreviewservicefactory.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_preview.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qqmlpreviewservice.json"

qt_internal_extend_target(QQmlPreviewServiceFactoryPlugin CONDITION QT_FEATURE_translation
    SOURCES
        proxytranslator.cpp proxytranslator.h
        qqmldebugtranslationservice.cpp qqmldebugtranslationservice.h
    LIBRARIES
        Qt::Gui
)
