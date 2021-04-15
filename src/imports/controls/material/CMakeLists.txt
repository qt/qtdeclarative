# Generated from material.pro.

#####################################################################
## qtquickcontrols2materialstyleplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquickcontrols2materialstyleplugin
    URI "QtQuick.Controls.Material"
    VERSION "${PROJECT_VERSION}"
    CLASSNAME QtQuickControls2MaterialStylePlugin
    IMPORTS
        QtQuick.Controls.Basic/auto
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        qquickmaterialstyle.cpp qquickmaterialstyle_p.h
        qquickmaterialtheme.cpp qquickmaterialtheme_p.h
        qtquickcontrols2materialstyleplugin.cpp
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
set(qtquickcontrols2materialstyleplugin_resource_files
    "images/arrow-indicator.png"
    "images/arrow-indicator@2x.png"
    "images/arrow-indicator@3x.png"
    "images/arrow-indicator@4x.png"
    "images/check.png"
    "images/check@2x.png"
    "images/check@3x.png"
    "images/check@4x.png"
    "images/drop-indicator.png"
    "images/drop-indicator@2x.png"
    "images/drop-indicator@3x.png"
    "images/drop-indicator@4x.png"
    "shaders/+glslcore/RectangularGlow.frag"
    "shaders/+hlsl/RectangularGlow.frag"
    "shaders/+qsb/RectangularGlow.frag"
    "shaders/RectangularGlow.frag"
)

qt_internal_add_resource(qtquickcontrols2materialstyleplugin "qtquickcontrols2materialstyleplugin"
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls/Material"
    FILES
        ${qtquickcontrols2materialstyleplugin_resource_files}
)


#### Keys ignored in scope 1:.:.:material.pro:<TRUE>:
# OTHER_FILES = "qmldir" "$$QML_FILES"
# QML_IMPORT_NAME = "QtQuick.Controls.Material"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick/Controls/Material"
# _REQUIREMENTS = "qtConfig(quickcontrols2-material)"

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
    "SwipeView.qml"
    "Switch.qml"
    "SwitchDelegate.qml"
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
set_source_files_properties(SwipeView.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(Switch.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(SwitchDelegate.qml PROPERTIES
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

qt6_target_qml_files(qtquickcontrols2materialstyleplugin
    FILES
        ${qml_files}
)
