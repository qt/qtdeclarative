# Generated from fusion-impl.pro.

#####################################################################
## qtquickcontrols2fusionstyleimplplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquickcontrols2fusionstyleimplplugin
    URI "QtQuick.Controls.Fusion.impl"
    VERSION "${PROJECT_VERSION}"
    CLASSNAME QtQuickControls2FusionStyleImplPlugin
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        qquickfusionbusyindicator.cpp qquickfusionbusyindicator_p.h
        qquickfusiondial.cpp qquickfusiondial_p.h
        qquickfusionknob.cpp qquickfusionknob_p.h
        qtquickcontrols2fusionstyleimplplugin.cpp
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

#### Keys ignored in scope 1:.:.:fusion-impl.pro:<TRUE>:
# OTHER_FILES = "qmldir" "$$QML_FILES"
# QML_FILES = "$$PWD/ButtonPanel.qml" "$$PWD/CheckIndicator.qml" "$$PWD/RadioIndicator.qml" "$$PWD/SliderGroove.qml" "$$PWD/SliderHandle.qml" "$$PWD/SwitchIndicator.qml"
# QML_IMPORT_NAME = "QtQuick.Controls.Fusion.impl"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# QT_FOR_CONFIG = "quickcontrols2-private"
# TARGETPATH = "QtQuick/Controls/Fusion/impl"
# _REQUIREMENTS = "qtConfig(quickcontrols2-fusion)"

set(qml_files
    "ButtonPanel.qml"
    "CheckIndicator.qml"
    "RadioIndicator.qml"
    "SliderGroove.qml"
    "SliderHandle.qml"
    "SwitchIndicator.qml"
)
set_source_files_properties(ButtonPanel.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.3"
)
set_source_files_properties(CheckIndicator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.3"
)
set_source_files_properties(RadioIndicator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.3"
)
set_source_files_properties(SliderGroove.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.3"
)
set_source_files_properties(SliderHandle.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.3"
)
set_source_files_properties(SwitchIndicator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.3"
)

qt6_target_qml_files(qtquickcontrols2fusionstyleimplplugin
    FILES
        ${qml_files}
)
