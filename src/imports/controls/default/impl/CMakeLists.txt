# Generated from default-impl.pro.

#####################################################################
## qtquickcontrols2defaultstyleimplplugin Plugin:
#####################################################################

qt_add_qml_module(qtquickcontrols2defaultstyleimplplugin
    URI "QtQuick.Controls.Default.impl"
    VERSION "${CMAKE_PROJECT_VERSION}"
    CLASSNAME QtQuickControls2DefaultStyleImplPlugin
    DEPENDENCIES
        QtQuick.Templates/6.0
        QtQuick.Controls/6.0
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
    SOURCES
        qquickdefaultbusyindicator.cpp qquickdefaultbusyindicator_p.h
        qquickdefaultdial.cpp qquickdefaultdial_p.h
        qquickdefaultprogressbar.cpp qquickdefaultprogressbar_p.h
        qtquickcontrols2defaultstyleimplplugin.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::QmlPrivate
        Qt::QuickControls2ImplPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:default-impl.pro:<TRUE>:
# OTHER_FILES = "qmldir"
# QML_IMPORT_NAME = "QtQuick.Controls.Default.impl"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# QT_FOR_CONFIG = "quickcontrols2-private"
# TARGETPATH = "QtQuick/Controls/Default/impl"
# _REQUIREMENTS = "qtConfig(quickcontrols2-default)"
