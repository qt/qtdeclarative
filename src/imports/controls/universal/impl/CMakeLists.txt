# Generated from universal-impl.pro.

#####################################################################
## qtquickcontrols2universalstyleimplplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquickcontrols2universalstyleimplplugin
    URI "QtQuick.Controls.Universal.impl"
    VERSION "${PROJECT_VERSION}"
    CLASSNAME QtQuickControls2UniversalStyleImplPlugin
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        qquickuniversalbusyindicator.cpp qquickuniversalbusyindicator_p.h
        qquickuniversalfocusrectangle.cpp qquickuniversalfocusrectangle_p.h
        qquickuniversalprogressbar.cpp qquickuniversalprogressbar_p.h
        qtquickcontrols2universalstyleimplplugin.cpp
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

#### Keys ignored in scope 1:.:.:universal-impl.pro:<TRUE>:
# OTHER_FILES = "qmldir" "$$QML_FILES"
# QML_FILES = "$$PWD/CheckIndicator.qml" "$$PWD/RadioIndicator.qml" "$$PWD/SwitchIndicator.qml"
# QML_IMPORT_NAME = "QtQuick.Controls.Universal.impl"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# QT_FOR_CONFIG = "quickcontrols2-private"
# TARGETPATH = "QtQuick/Controls/Universal/impl"
# _REQUIREMENTS = "qtConfig(quickcontrols2-universal)"

set(qml_files
    "CheckIndicator.qml"
    "RadioIndicator.qml"
    "SwitchIndicator.qml"
)
set_source_files_properties(CheckIndicator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0"
)
set_source_files_properties(RadioIndicator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0"
)
set_source_files_properties(SwitchIndicator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0"
)

qt6_target_qml_files(qtquickcontrols2universalstyleimplplugin
    FILES
        ${qml_files}
)
