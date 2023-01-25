# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(QuickLayouts
    URI "QtQuick.Layouts"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    PLUGIN_TARGET qquicklayoutsplugin
    CLASS_NAME QtQuickLayoutsPlugin
    DEPENDENCIES
        QtQuick/auto
    SOURCES
        qquickgridlayoutengine.cpp qquickgridlayoutengine_p.h
        qquicklayout.cpp qquicklayout_p.h
        qquicklayoutstyleinfo.cpp qquicklayoutstyleinfo_p.h
        qquicklinearlayout.cpp qquicklinearlayout_p.h
        qquickstacklayout.cpp qquickstacklayout_p.h
        qquicklayoutitemproxy.cpp qquicklayoutitemproxy_p.h
        qquicklayoutglobal_p.h
    DEFINES
        QT_BUILD_QUICKLAYOUTS_LIB
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::GuiPrivate
        Qt::QuickPrivate
        Qt::Qml
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)
