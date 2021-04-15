# Generated from quickcontrols2impl.pro.

#####################################################################
## QuickControls2Impl Module:
#####################################################################

qt_internal_add_module(QuickControls2Impl
    GENERATE_METATYPES
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
        qtquickcontrols2foreign_p.h
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
)

#### Keys ignored in scope 1:.:.:quickcontrols2impl.pro:<TRUE>:
# MODULE = "quickcontrols2impl"
# QMLTYPES_FILENAME = "plugins.qmltypes"
# QMLTYPES_INSTALL_DIR = "$$[QT_INSTALL_QML]/QtQuick/Controls/impl"
# QML_IMPORT_NAME = "QtQuick.Controls.impl"
# QML_IMPORT_VERSION = "$$QT_VERSION"

## Scopes:
#####################################################################

qt_internal_extend_target(QuickControls2Impl CONDITION QT_FEATURE_quick_listview AND QT_FEATURE_quick_pathview
    SOURCES
        qquicktumblerview.cpp qquicktumblerview_p.h
)

set_target_properties(QuickControls2Impl PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${PROJECT_VERSION}
    QT_QML_MODULE_URI QtQuick.Controls.impl
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtQuick/Controls/impl"
)

qt6_qml_type_registration(QuickControls2Impl)
