#####################################################################
## qtquickcontrols2macosstyleplugin Plugin:
#####################################################################

set(qml_files
    "Button.qml"
    "Slider.qml"
    "GroupBox.qml"
    "CheckBox.qml"
    "RadioButton.qml"
    "SpinBox.qml"
    "TextField.qml"
    "Frame.qml"
    "TextArea.qml"
    "ComboBox.qml"
    "ScrollView.qml"
    "ScrollBar.qml"
    "ProgressBar.qml"
    "Dial.qml"
)
set_source_files_properties(${qml_files} PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)

qt_internal_add_qml_module(qtquickcontrols2macosstyleplugin
    URI "QtQuick.Controls.macOS"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtQuickControls2MacOSStylePlugin
    IMPORTS
        QtQuick.Controls.Fusion/auto
    PAST_MAJOR_VERSIONS 2
    PLUGIN_TARGET qtquickcontrols2macosstyleplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
        qtquickcontrols2macosstyleplugin.cpp
    QML_FILES
        ${qml_files}
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2Private
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
)
