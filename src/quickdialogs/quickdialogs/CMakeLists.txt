# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QuickDialogs2 Module:
#####################################################################

qt_internal_add_qml_module(QuickDialogs2
    URI "QtQuick.Dialogs"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtQuickDialogsPlugin
    PLUGIN_TARGET qtquickdialogsplugin
    DEPENDENCIES
        QtQuick/auto
        QtQuick.Dialogs.quickimpl/auto
    SOURCES
        qquickabstractdialog.cpp
        qquickabstractdialog_p.h
        qquickcolordialog.cpp
        qquickcolordialog_p.h
        qquickfiledialog.cpp
        qquickfiledialog_p.h
        qquickfolderdialog.cpp
        qquickfolderdialog_p.h
        qquickfontdialog.cpp
        qquickfontdialog_p.h
        qquickmessagedialog.cpp
        qquickmessagedialog_p.h
        qtquickdialogs2foreign.cpp
        qtquickdialogs2foreign_p.h
        qtquickdialogs2global_p.h
    DEFINES
        QT_BUILD_QUICKDIALOGS2_LIB
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickControls2Impl
        Qt::QuickDialogs2Utils
        Qt::QuickDialogs2UtilsPrivate
        Qt::QuickDialogs2QuickImpl
        Qt::QuickDialogs2QuickImplPrivate
        Qt::QuickTemplates2Private
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
    )

qt_internal_add_docs(QuickDialogs2
    doc/qtquickdialogs.qdocconf
)
