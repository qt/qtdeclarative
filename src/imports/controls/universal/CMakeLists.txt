# Generated from universal.pro.

#####################################################################
## qtquickcontrols2universalstyleplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquickcontrols2universalstyleplugin
    URI "QtQuick.Controls.Universal"
    VERSION "${PROJECT_VERSION}"
    CLASSNAME QtQuickControls2UniversalStylePlugin
    IMPORTS
        QtQuick.Controls.Basic/auto
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        qquickuniversalstyle.cpp qquickuniversalstyle_p.h
        qquickuniversaltheme.cpp qquickuniversaltheme_p.h
        qtquickcontrols2universalstyleplugin.cpp
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

# Resources:
set(qtquickcontrols2universalstyleplugin_resource_files
    "images/checkmark.png"
    "images/checkmark@2x.png"
    "images/checkmark@3x.png"
    "images/checkmark@4x.png"
    "images/downarrow.png"
    "images/downarrow@2x.png"
    "images/downarrow@3x.png"
    "images/downarrow@4x.png"
    "images/leftarrow.png"
    "images/leftarrow@2x.png"
    "images/leftarrow@3x.png"
    "images/leftarrow@4x.png"
    "images/rightarrow.png"
    "images/rightarrow@2x.png"
    "images/rightarrow@3x.png"
    "images/rightarrow@4x.png"
)

qt_internal_add_resource(qtquickcontrols2universalstyleplugin "qtquickcontrols2universalstyleplugin"
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls/Universal"
    FILES
        ${qtquickcontrols2universalstyleplugin_resource_files}
)


#### Keys ignored in scope 1:.:.:universal.pro:<TRUE>:
# OTHER_FILES = "qmldir" "$$QML_FILES"
# QML_IMPORT_NAME = "QtQuick.Controls.Universal"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick/Controls/Universal"
# _REQUIREMENTS = "qtConfig(quickcontrols2-universal)"

set(qml_files
    "ApplicationWindow.qml"
    "BusyIndicator.qml"
    "Button.qml"
    "CheckBox.qml"
    "CheckDelegate.qml"
    "ComboBox.qml"
    "DelayButton.qml"
    "Dial.qml"
    "Dialog.qml"
    "DialogButtonBox.qml"
    "Drawer.qml"
    "Frame.qml"
    "GroupBox.qml"
    "HorizontalHeaderView.qml"
    "ItemDelegate.qml"
    "Label.qml"
    "Menu.qml"
    "MenuBar.qml"
    "MenuBarItem.qml"
    "MenuItem.qml"
    "MenuSeparator.qml"
    "Page.qml"
    "PageIndicator.qml"
    "Pane.qml"
    "Popup.qml"
    "ProgressBar.qml"
    "RadioButton.qml"
    "RadioDelegate.qml"
    "RangeSlider.qml"
    "RoundButton.qml"
    "ScrollView.qml"
    "ScrollBar.qml"
    "ScrollIndicator.qml"
    "Slider.qml"
    "SpinBox.qml"
    "SplitView.qml"
    "StackView.qml"
    "SwipeDelegate.qml"
    "SwitchDelegate.qml"
    "Switch.qml"
    "TabBar.qml"
    "TabButton.qml"
    "TextArea.qml"
    "TextField.qml"
    "ToolBar.qml"
    "ToolButton.qml"
    "ToolSeparator.qml"
    "ToolTip.qml"
    "Tumbler.qml"
    "VerticalHeaderView.qml"
)
set_source_files_properties(ApplicationWindow.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(BusyIndicator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(Button.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(CheckBox.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(CheckDelegate.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(ComboBox.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(DelayButton.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.2;6.0"
)
set_source_files_properties(Dial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(Dialog.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.1;6.0"
)
set_source_files_properties(DialogButtonBox.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.1;6.0"
)
set_source_files_properties(Drawer.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(Frame.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(GroupBox.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(HorizontalHeaderView.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.15;6.0"
)
set_source_files_properties(ItemDelegate.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(Label.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(Menu.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(MenuBar.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.3;6.0"
)
set_source_files_properties(MenuBarItem.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.3;6.0"
)
set_source_files_properties(MenuItem.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(MenuSeparator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.1;6.0"
)
set_source_files_properties(Page.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(PageIndicator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(Pane.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(Popup.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(ProgressBar.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(RadioButton.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(RadioDelegate.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(RangeSlider.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(RoundButton.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.1;6.0"
)
set_source_files_properties(ScrollView.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(ScrollBar.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(ScrollIndicator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(Slider.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(SpinBox.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(SplitView.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.13;6.0"
)
set_source_files_properties(StackView.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(SwipeDelegate.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(SwitchDelegate.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(Switch.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(TabBar.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(TabButton.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(TextArea.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(TextField.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(ToolBar.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(ToolButton.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(ToolSeparator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.1;6.0"
)
set_source_files_properties(ToolTip.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(Tumbler.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(VerticalHeaderView.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.15;6.0"
)

qt6_target_qml_files(qtquickcontrols2universalstyleplugin
    FILES
        ${qml_files}
)
