# Generated from macos.pro.

#####################################################################
## qtquickcontrols2macosstyleplugin Plugin:
#####################################################################

qt_add_qml_module(qtquickcontrols2macosstyleplugin
    TARGET_PATH "QtQuick/Controls.2/macOS"
    URI "QtQuick.Controls.macOS"
    VERSION "2.0"
    CLASSNAME QtQuickControls2MacOSStylePlugin
    DEPENDENCIES
        QtQuick.Controls/2.5
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
    SOURCES
        qtquickcontrols2macosstyleplugin.cpp
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
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickTemplates2
)

#### Keys ignored in scope 1:.:.:macos.pro:<TRUE>:
# OTHER_FILES = "qmldir" "$$QML_FILES"
# QML_IMPORT_MAJOR_VERSION = "2"
# QML_IMPORT_NAME = "QtQuick.Controls.macOS"
# TARGETPATH = "QtQuick/Controls.2/macOS"
# _REQUIREMENTS = "qtConfig(quickcontrols2-macos)"

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
    "ScrollBar.qml"
    "ProgressBar.qml"
    "Dial.qml"
)
set_source_files_properties(Button.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Slider.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(GroupBox.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(CheckBox.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(RadioButton.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(SpinBox.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(TextField.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Frame.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(TextArea.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ComboBox.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ScrollBar.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ProgressBar.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Dial.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)

qt6_target_qml_files(qtquickcontrols2macosstyleplugin
    FILES
        ${qml_files}
)
