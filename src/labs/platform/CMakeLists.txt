# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(LabsPlatform
    URI "Qt.labs.platform"
    VERSION "1.1"
    CLASS_NAME QtLabsPlatformPlugin
    DEPENDENCIES
        QtQuick
        QtQuick.Templates
    SOURCES
        qquicklabsplatformcolordialog.cpp qquicklabsplatformcolordialog_p.h
        qquicklabsplatformdialog.cpp qquicklabsplatformdialog_p.h
        qquicklabsplatformfiledialog.cpp qquicklabsplatformfiledialog_p.h
        qquicklabsplatformfolderdialog.cpp qquicklabsplatformfolderdialog_p.h
        qquicklabsplatformfontdialog.cpp qquicklabsplatformfontdialog_p.h
        qquicklabsplatformicon.cpp qquicklabsplatformicon_p.h
        qquicklabsplatformiconloader.cpp qquicklabsplatformiconloader_p.h
        qquicklabsplatformmenu.cpp qquicklabsplatformmenu_p.h
        qquicklabsplatformmenubar.cpp qquicklabsplatformmenubar_p.h
        qquicklabsplatformmenuitem.cpp qquicklabsplatformmenuitem_p.h
        qquicklabsplatformmenuitemgroup.cpp qquicklabsplatformmenuitemgroup_p.h
        qquicklabsplatformmenuseparator.cpp qquicklabsplatformmenuseparator_p.h
        qquicklabsplatformmessagedialog.cpp qquicklabsplatformmessagedialog_p.h
        qquicklabsplatformstandardpaths.cpp qquicklabsplatformstandardpaths_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
)

qt_internal_extend_target(LabsPlatform
    CONDITION
        QT_FEATURE_systemtrayicon
    SOURCES
        qquicklabsplatformsystemtrayicon.cpp qquicklabsplatformsystemtrayicon_p.h
)

if(TARGET Qt::Widgets)

    qt_internal_extend_target(LabsPlatform
        SOURCES
            widgets/qwidgetplatform_p.h
        LIBRARIES
            Qt::Widgets
    )

    qt_internal_extend_target(LabsPlatform
        CONDITION
            QT_FEATURE_systemtrayicon
        SOURCES
            widgets/qwidgetplatformsystemtrayicon.cpp widgets/qwidgetplatformsystemtrayicon_p.h
    )

    qt_internal_extend_target(LabsPlatform
        CONDITION
            QT_FEATURE_colordialog OR
            QT_FEATURE_filedialog OR
            QT_FEATURE_fontdialog OR
            QT_FEATURE_messagebox
        SOURCES
            widgets/qwidgetplatformdialog.cpp widgets/qwidgetplatformdialog_p.h
    )

    qt_internal_extend_target(LabsPlatform
        CONDITION
            QT_FEATURE_colordialog
        SOURCES
            widgets/qwidgetplatformcolordialog.cpp widgets/qwidgetplatformcolordialog_p.h
    )

    qt_internal_extend_target(LabsPlatform
        CONDITION
            QT_FEATURE_filedialog
        SOURCES
            widgets/qwidgetplatformfiledialog.cpp widgets/qwidgetplatformfiledialog_p.h
    )

    qt_internal_extend_target(LabsPlatform
        CONDITION
            QT_FEATURE_fontdialog
        SOURCES
            widgets/qwidgetplatformfontdialog.cpp widgets/qwidgetplatformfontdialog_p.h
    )

    qt_internal_extend_target(LabsPlatform
        CONDITION
            QT_FEATURE_menu
        SOURCES
            widgets/qwidgetplatformmenu.cpp widgets/qwidgetplatformmenu_p.h
            widgets/qwidgetplatformmenuitem.cpp widgets/qwidgetplatformmenuitem_p.h
    )

    qt_internal_extend_target(LabsPlatform
        CONDITION
            QT_FEATURE_messagebox
        SOURCES
            widgets/qwidgetplatformmessagedialog.cpp widgets/qwidgetplatformmessagedialog_p.h
    )
endif()

qt_internal_add_docs(LabsPlatform
    doc/qtlabsplatform.qdocconf
)
