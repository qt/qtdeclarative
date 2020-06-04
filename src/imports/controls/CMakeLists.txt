# Generated from controls.pro.

#####################################################################
## qtquickcontrols2plugin Plugin:
#####################################################################

qt_add_qml_module(qtquickcontrols2plugin
    TARGET_PATH "QtQuick/Controls.2"
    URI "QtQuick.Controls"
    VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuickControls2Plugin
    DEPENDENCIES
        QtQuick.Templates/2.5
    SKIP_TYPE_REGISTRATION
    SOURCES
        qquickdefaultbusyindicator.cpp qquickdefaultbusyindicator_p.h
        qquickdefaultdial.cpp qquickdefaultdial_p.h
        qquickdefaultprogressbar.cpp qquickdefaultprogressbar_p.h
        qquickdefaultstyle.cpp qquickdefaultstyle_p.h
        qquickdefaulttheme.cpp qquickdefaulttheme_p.h
        qtquickcontrols2plugin.cpp
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
)

# Resources:
set(qtquickcontrols2plugin_resource_files
    "images/arrow-indicator.png"
    "images/arrow-indicator@2x.png"
    "images/arrow-indicator@3x.png"
    "images/arrow-indicator@4x.png"
    "images/check.png"
    "images/check@2x.png"
    "images/check@3x.png"
    "images/check@4x.png"
    "images/dial-indicator.png"
    "images/dial-indicator@2x.png"
    "images/dial-indicator@3x.png"
    "images/dial-indicator@4x.png"
    "images/double-arrow.png"
    "images/double-arrow@2x.png"
    "images/double-arrow@3x.png"
    "images/double-arrow@4x.png"
    "images/drop-indicator.png"
    "images/drop-indicator@2x.png"
    "images/drop-indicator@3x.png"
    "images/drop-indicator@4x.png"
)

qt_add_resource(qtquickcontrols2plugin "qtquickcontrols2plugin"
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls.2"
    FILES
        ${qtquickcontrols2plugin_resource_files}
)


#### Keys ignored in scope 1:.:.:controls.pro:<TRUE>:
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# OTHER_FILES = "qmldir" "$$QML_FILES" "$$PWD/snippets/*.qml" "$$PWD/src/*.qdoc" "$$PWD/src/templates/*.qdoc"
# TARGETPATH = "QtQuick/Controls.2"
# _REQUIREMENTS = "qtConfig(quickcontrols2-default)"

## Scopes:
#####################################################################

set(qml_files
    "AbstractButton.qml"
    "Action.qml"
    "ActionGroup.qml"
    "ApplicationWindow.qml"
    "BusyIndicator.qml"
    "Button.qml"
    "ButtonGroup.qml"
    "CheckBox.qml"
    "CheckDelegate.qml"
    "ComboBox.qml"
    "Container.qml"
    "Control.qml"
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
    "ScrollBar.qml"
    "ScrollIndicator.qml"
    "ScrollView.qml"
    "Slider.qml"
    "SpinBox.qml"
    "SplitView.qml"
    "StackView.qml"
    "SwipeDelegate.qml"
    "Switch.qml"
    "SwitchDelegate.qml"
    "SwipeView.qml"
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
set_source_files_properties(AbstractButton.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Action.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ActionGroup.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ApplicationWindow.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(BusyIndicator.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Button.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ButtonGroup.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(CheckBox.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(CheckDelegate.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ComboBox.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Container.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Control.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(DelayButton.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Dial.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Dialog.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(DialogButtonBox.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Drawer.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Frame.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(GroupBox.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(HorizontalHeaderView.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ItemDelegate.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Label.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Menu.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(MenuBar.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(MenuBarItem.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(MenuItem.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(MenuSeparator.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Page.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(PageIndicator.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Pane.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Popup.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ProgressBar.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(RadioButton.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(RadioDelegate.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(RangeSlider.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(RoundButton.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ScrollBar.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ScrollIndicator.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ScrollView.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Slider.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(SpinBox.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(SplitView.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(StackView.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(SwipeDelegate.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Switch.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(SwitchDelegate.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(SwipeView.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(TabBar.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(TabButton.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(TextArea.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(TextField.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ToolBar.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ToolButton.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ToolSeparator.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(ToolTip.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(Tumbler.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(VerticalHeaderView.qml PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)

qt6_target_qml_files(qtquickcontrols2plugin
    FILES
        ${qml_files}
)
qt_add_docs(qtquickcontrols2plugin
    doc/qtquickcontrols.qdocconf
)

