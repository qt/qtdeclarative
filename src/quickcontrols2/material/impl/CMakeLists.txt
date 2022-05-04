#####################################################################
## qtquickcontrols2materialstyleimplplugin Plugin:
#####################################################################

set(qml_files
    "BoxShadow.qml"
    "CheckIndicator.qml"
    "CursorDelegate.qml"
    "ElevationEffect.qml"
    "RadioIndicator.qml"
    "RectangularGlow.qml"
    "SliderHandle.qml"
    "SwitchIndicator.qml"
)

qt_internal_add_qml_module(qtquickcontrols2materialstyleimplplugin
    URI "QtQuick.Controls.Material.impl"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    CLASS_NAME QtQuickControls2MaterialStyleImplPlugin
    DEPENDENCIES
        QtQuick/auto
    PLUGIN_TARGET qtquickcontrols2materialstyleimplplugin
    NO_PLUGIN_OPTIONAL
    SOURCES
        qquickmaterialbusyindicator.cpp qquickmaterialbusyindicator_p.h
        qquickmaterialprogressbar.cpp qquickmaterialprogressbar_p.h
        qquickmaterialripple.cpp qquickmaterialripple_p.h
    QML_FILES
        ${qml_files}
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
)
