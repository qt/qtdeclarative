# Generated from quickcontrols2.pro.

#####################################################################
## QuickControls2 Module:
#####################################################################

qt_add_module(QuickControls2
    SOURCES
        qquickanimatednode.cpp qquickanimatednode_p.h
        qquickattachedobject.cpp qquickattachedobject_p.h
        qquickchecklabel.cpp qquickchecklabel_p.h
        qquickclippedtext.cpp qquickclippedtext_p.h
        qquickcolor.cpp qquickcolor_p.h
        qquickcolorimage.cpp qquickcolorimage_p.h
        qquickiconimage.cpp qquickiconimage_p.h
        qquickiconimage_p_p.h
        qquickiconlabel.cpp qquickiconlabel_p.h
        qquickiconlabel_p_p.h
        qquickitemgroup.cpp qquickitemgroup_p.h
        qquickmnemoniclabel.cpp qquickmnemoniclabel_p.h
        qquickpaddedrectangle.cpp qquickpaddedrectangle_p.h
        qquickplaceholdertext.cpp qquickplaceholdertext_p.h
        qquickstyle.cpp qquickstyle.h qquickstyle_p.h
        qquickstyleplugin.cpp qquickstyleplugin_p.h
        qquickstyleselector.cpp qquickstyleselector_p.h
        qquickstyleselector_p_p.h
        qtquickcontrols2global.h qtquickcontrols2global_p.h
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
)

#### Keys ignored in scope 1:.:.:quickcontrols2.pro:<TRUE>:
# MODULE = "quickcontrols2"

## Scopes:
#####################################################################

qt_extend_target(QuickControls2 CONDITION QT_FEATURE_quick_listview AND QT_FEATURE_quick_pathview
    SOURCES
        qquicktumblerview.cpp qquicktumblerview_p.h
)
