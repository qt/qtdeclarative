# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QuickControls2Impl Module:
#####################################################################

qt_internal_add_qml_module(QuickControls2Impl
    URI "QtQuick.Controls.impl"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtQuickControls2ImplPlugin
    DEPENDENCIES
        QtQuick/auto
        QtQuick.Templates/auto
    PLUGIN_TARGET qtquickcontrols2implplugin
    SOURCES
        qquickanimatednode.cpp qquickanimatednode_p.h
        qquickchecklabel.cpp qquickchecklabel_p.h
        qquickclippedtext.cpp qquickclippedtext_p.h
        qquickcolor.cpp qquickcolor_p.h
        qquickcolorimage.cpp qquickcolorimage_p.h
        qquickiconimage.cpp qquickiconimage_p.h
        qquickiconimage_p_p.h
        qquickninepatchimage.cpp qquickninepatchimage_p.h
        qquickimageselector.cpp qquickimageselector_p.h
        qquickiconlabel.cpp qquickiconlabel_p.h
        qquickiconlabel_p_p.h
        qquickitemgroup.cpp qquickitemgroup_p.h
        qquickmnemoniclabel.cpp qquickmnemoniclabel_p.h
        qquickpaddedrectangle.cpp qquickpaddedrectangle_p.h
        qquickplaceholdertext.cpp qquickplaceholdertext_p.h
        qquickplatformtheme.cpp qquickplatformtheme_p.h
        qtquickcontrols2foreign.cpp qtquickcontrols2foreign_p.h
        qtquickcontrols2implglobal_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

qt_internal_extend_target(QuickControls2Impl CONDITION QT_FEATURE_quick_listview AND QT_FEATURE_quick_pathview
    SOURCES
        qquicktumblerview.cpp qquicktumblerview_p.h
)
