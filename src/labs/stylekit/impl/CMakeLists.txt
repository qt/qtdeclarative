# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## StyleKitImpl Module:
#####################################################################

qt_internal_add_qml_module(QtQuickStyleKitImpl
    URI Qt.labs.StyleKit.impl
    DEPENDENCIES
        QtQuick/auto
    SOURCES
        qqstylekitdelegatecontainer_p.h qqstylekitdelegatecontainer.cpp
        qqstylekitlayout_p.h qqstylekitlayout.cpp
    QML_FILES
        BackgroundAndIndicatorDelegate.qml
        BackgroundDelegate.qml
        FallbackStyle.qml
        HandleDelegate.qml
        IndicatorDelegate.qml
        Shadow.qml
    LIBRARIES
        Qt6::GuiPrivate
        Qt6::Quick
        Qt6::QuickPrivate
        Qt6::QuickTemplates2
        Qt6::QuickTemplates2Private
        Qt6::QtQuickStyleKit
)
