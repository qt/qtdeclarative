# Generated from platform.pro.

#####################################################################
## qtlabsplatformplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtlabsplatformplugin
    URI "Qt.labs.platform"
    VERSION "1.1"
    CLASSNAME QtLabsPlatformPlugin
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        qquickplatformcolordialog.cpp qquickplatformcolordialog_p.h
        qquickplatformdialog.cpp qquickplatformdialog_p.h
        qquickplatformfiledialog.cpp qquickplatformfiledialog_p.h
        qquickplatformfolderdialog.cpp qquickplatformfolderdialog_p.h
        qquickplatformfontdialog.cpp qquickplatformfontdialog_p.h
        qquickplatformicon.cpp qquickplatformicon_p.h
        qquickplatformiconloader.cpp qquickplatformiconloader_p.h
        qquickplatformmenu.cpp qquickplatformmenu_p.h
        qquickplatformmenubar.cpp qquickplatformmenubar_p.h
        qquickplatformmenuitem.cpp qquickplatformmenuitem_p.h
        qquickplatformmenuitemgroup.cpp qquickplatformmenuitemgroup_p.h
        qquickplatformmenuseparator.cpp qquickplatformmenuseparator_p.h
        qquickplatformmessagedialog.cpp qquickplatformmessagedialog_p.h
        qquickplatformstandardpaths.cpp qquickplatformstandardpaths_p.h
        qtlabsplatformplugin.cpp
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
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:platform.pro:<TRUE>:
# IMPORT_VERSION = "1.1"
# OTHER_FILES = "qmldir"
# TARGETPATH = "Qt/labs/platform"

## Scopes:
#####################################################################

qt_internal_extend_target(qtlabsplatformplugin CONDITION TARGET Qt::Widgets
    SOURCES
        widgets/qwidgetplatform_p.h
    PUBLIC_LIBRARIES
        Qt::Widgets
)

qt_internal_extend_target(qtlabsplatformplugin CONDITION TARGET Qt::Widgets AND QT_FEATURE_systemtrayicon AND TARGET Qt::Widgets # special case
    SOURCES
        widgets/qwidgetplatformsystemtrayicon.cpp widgets/qwidgetplatformsystemtrayicon_p.h
)

qt_internal_extend_target(qtlabsplatformplugin CONDITION TARGET Qt::Widgets AND (QT_FEATURE_colordialog OR QT_FEATURE_filedialog OR QT_FEATURE_fontdialog OR QT_FEATURE_messagebox)
    SOURCES
        widgets/qwidgetplatformdialog.cpp widgets/qwidgetplatformdialog_p.h
)

qt_internal_extend_target(qtlabsplatformplugin CONDITION TARGET Qt::Widgets AND QT_FEATURE_colordialog AND TARGET Qt::Widgets # special case
    SOURCES
        widgets/qwidgetplatformcolordialog.cpp widgets/qwidgetplatformcolordialog_p.h
)

qt_internal_extend_target(qtlabsplatformplugin CONDITION TARGET Qt::Widgets AND QT_FEATURE_filedialog AND TARGET Qt::Widgets # special case
    SOURCES
        widgets/qwidgetplatformfiledialog.cpp widgets/qwidgetplatformfiledialog_p.h
)

qt_internal_extend_target(qtlabsplatformplugin CONDITION TARGET Qt::Widgets AND QT_FEATURE_fontdialog AND TARGET Qt::Widgets # special case
    SOURCES
        widgets/qwidgetplatformfontdialog.cpp widgets/qwidgetplatformfontdialog_p.h
)

qt_internal_extend_target(qtlabsplatformplugin CONDITION TARGET Qt::Widgets AND QT_FEATURE_menu AND TARGET Qt::Widgets # special case
    SOURCES
        widgets/qwidgetplatformmenu.cpp widgets/qwidgetplatformmenu_p.h
        widgets/qwidgetplatformmenuitem.cpp widgets/qwidgetplatformmenuitem_p.h
)

qt_internal_extend_target(qtlabsplatformplugin CONDITION TARGET Qt::Widgets AND QT_FEATURE_messagebox AND TARGET Qt::Widgets # special case
    SOURCES
        widgets/qwidgetplatformmessagedialog.cpp widgets/qwidgetplatformmessagedialog_p.h
)

qt_internal_extend_target(qtlabsplatformplugin CONDITION QT_FEATURE_systemtrayicon
    SOURCES
        qquickplatformsystemtrayicon.cpp qquickplatformsystemtrayicon_p.h
)
qt_internal_add_docs(qtlabsplatformplugin
    doc/qtlabsplatform.qdocconf
)

