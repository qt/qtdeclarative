# Generated from basic-impl.pro.

#####################################################################
## qtquickcontrols2basicstyleimplplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquickcontrols2basicstyleimplplugin
    URI "QtQuick.Controls.Basic.impl"
    VERSION "${PROJECT_VERSION}"
    CLASSNAME QtQuickControls2BasicStyleImplPlugin
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        qquickbasicbusyindicator.cpp qquickbasicbusyindicator_p.h
        qquickbasicdial.cpp qquickbasicdial_p.h
        qquickbasicprogressbar.cpp qquickbasicprogressbar_p.h
        qtquickcontrols2basicstyleimplplugin.cpp
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

#### Keys ignored in scope 1:.:.:basic-impl.pro:<TRUE>:
# OTHER_FILES = "qmldir"
# QML_IMPORT_NAME = "QtQuick.Controls.Basic.impl"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# QT_FOR_CONFIG = "quickcontrols2-private"
# TARGETPATH = "QtQuick/Controls/Basic/impl"
# _REQUIREMENTS = "qtConfig(quickcontrols2-basic)"
