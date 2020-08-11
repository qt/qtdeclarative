# Generated from windows.pro.

#####################################################################
## qtquickcontrols2windowsstyleplugin Plugin:
#####################################################################

qt_add_qml_module(qtquickcontrols2windowsstyleplugin
    TARGET_PATH "QtQuick/Controls.2/Windows"
    URI "QtQuick.Controls.Windows"
    VERSION "6.0"
    CLASSNAME QtQuickControls2WindowsStylePlugin
    DEPENDENCIES
        QtQuick.Controls/2.5
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
    SOURCES
        qtquickcontrols2windowsstyleplugin.cpp
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

#### Keys ignored in scope 1:.:.:windows.pro:<TRUE>:
# OTHER_FILES = "qmldir" "$$QML_FILES"
# QML_IMPORT_MAJOR_VERSION = "6"
# QML_IMPORT_NAME = "QtQuick.Controls.Windows"
# TARGETPATH = "QtQuick/Controls.2/Windows"
# _REQUIREMENTS = "qtConfig(quickcontrols2-windows)"

set(qml_files
    "Button.qml"
    "CheckBox.qml"
    "ComboBox.qml"
    "Frame.qml"
    "GroupBox.qml"
    "RadioButton.qml"
    "Slider.qml"
    "SpinBox.qml"
    "TextArea.qml"
    "TextField.qml"
)
set_source_files_properties(Button.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(CheckBox.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ComboBox.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Frame.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(GroupBox.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(RadioButton.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Slider.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(SpinBox.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(TextArea.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(TextField.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)

qt6_target_qml_files(qtquickcontrols2windowsstyleplugin
    FILES
        ${qml_files}
)
