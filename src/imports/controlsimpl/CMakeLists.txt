# Generated from controlsimpl.pro.

#####################################################################
## qtquickcontrols2implplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquickcontrols2implplugin
    URI "QtQuick.Controls.impl"
    VERSION "${PROJECT_VERSION}"
    CLASSNAME QtQuickControls2ImplPlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        qtquickcontrols2implplugin.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2ImplPrivate
        Qt::QuickControls2Private
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:controlsimpl.pro:<TRUE>:
# OTHER_FILES = "qmldir"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick/Controls/impl"
