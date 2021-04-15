# Generated from material-impl.pro.

#####################################################################
## qtquickcontrols2materialstyleimplplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquickcontrols2materialstyleimplplugin
    URI "QtQuick.Controls.Material.impl"
    VERSION "${PROJECT_VERSION}"
    CLASSNAME QtQuickControls2MaterialStyleImplPlugin
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        qquickmaterialbusyindicator.cpp qquickmaterialbusyindicator_p.h
        qquickmaterialprogressbar.cpp qquickmaterialprogressbar_p.h
        qquickmaterialripple.cpp qquickmaterialripple_p.h
        qtquickcontrols2materialstyleimplplugin.cpp
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

#### Keys ignored in scope 1:.:.:material-impl.pro:<TRUE>:
# OTHER_FILES = "qmldir" "$$QML_FILES"
# QML_FILES = "$$PWD/BoxShadow.qml" "$$PWD/CheckIndicator.qml" "$$PWD/CursorDelegate.qml" "$$PWD/ElevationEffect.qml" "$$PWD/RadioIndicator.qml" "$$PWD/RectangularGlow.qml" "$$PWD/SliderHandle.qml" "$$PWD/SwitchIndicator.qml"
# QML_IMPORT_NAME = "QtQuick.Controls.Material.impl"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# QT_FOR_CONFIG = "quickcontrols2-private"
# TARGETPATH = "QtQuick/Controls/Material/impl"
# _REQUIREMENTS = "qtConfig(quickcontrols2-material)"

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
set_source_files_properties(BoxShadow.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0"
)
set_source_files_properties(CheckIndicator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0"
)
set_source_files_properties(CursorDelegate.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0"
)
set_source_files_properties(ElevationEffect.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0"
)
set_source_files_properties(RadioIndicator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0"
)
set_source_files_properties(RectangularGlow.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0"
)
set_source_files_properties(SliderHandle.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0"
)
set_source_files_properties(SwitchIndicator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0"
)

qt6_target_qml_files(qtquickcontrols2materialstyleimplplugin
    FILES
        ${qml_files}
)
