# Generated from quickcontrols2.pro.

#####################################################################
## QuickControls2 Module:
#####################################################################

qt_internal_add_module(QuickControls2
    GENERATE_METATYPES
    SOURCES
        qquickdummyregistration_p.h
        qquickstyle.cpp qquickstyle.h qquickstyle_p.h
        qquickstyleplugin.cpp qquickstyleplugin_p.h
        qtquickcontrols2global.h
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
# QMLTYPES_FILENAME = "plugins.qmltypes"
# QMLTYPES_INSTALL_DIR = "$$[QT_INSTALL_QML]/QtQuick/Controls"
# QML_IMPORT_NAME = "QtQuick.Controls"
# QML_IMPORT_VERSION = "$$QT_VERSION"

set_target_properties(QuickControls2 PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${PROJECT_VERSION}
    QT_QML_MODULE_URI QtQuick.Controls
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtQuick/Controls"
)

qt6_qml_type_registration(QuickControls2)
