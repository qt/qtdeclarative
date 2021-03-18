# Generated from imagine.pro.

#####################################################################
## qtquickcontrols2imaginestyleplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquickcontrols2imaginestyleplugin
    URI "QtQuick.Controls.Imagine"
    VERSION "${CMAKE_PROJECT_VERSION}"
    CLASSNAME QtQuickControls2ImagineStylePlugin
    IMPORTS
        QtQuick.Controls.Basic/auto
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        qquickimaginestyle.cpp qquickimaginestyle_p.h
        qquickimaginetheme.cpp qquickimaginetheme_p.h
        qtquickcontrols2imaginestyleplugin.cpp
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

file(GLOB resource_glob_0 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "images/*.png")
foreach(file IN LISTS resource_glob_0)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

file(GLOB resource_glob_1 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "images/*.webp")
foreach(file IN LISTS resource_glob_1)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

# Resources:
set(qmake_qtquickcontrols2imaginestyle_resource_files
    ${resource_glob_0}
    ${resource_glob_1}
)

qt_internal_add_resource(qtquickcontrols2imaginestyleplugin "qmake_qtquickcontrols2imaginestyle"
    PREFIX
        "qt-project.org/imports/QtQuick/Controls/Imagine"
    FILES
        ${qmake_qtquickcontrols2imaginestyle_resource_files}
)


#### Keys ignored in scope 1:.:.:imagine.pro:<TRUE>:
# OTHER_FILES = "qmldir" "$$QML_FILES"
# QML_IMPORT_NAME = "QtQuick.Controls.Imagine"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick/Controls/Imagine"
# _REQUIREMENTS = "qtConfig(quickcontrols2-imagine)"

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
    "MenuItem.qml"
    "MenuSeparator.qml"
    "PageIndicator.qml"
    "Page.qml"
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
    "TextField.qml"
    "TextArea.qml"
    "TabBar.qml"
    "TabButton.qml"
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
set_source_files_properties(MenuItem.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(MenuSeparator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.1;6.0"
)
set_source_files_properties(PageIndicator.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(Page.qml PROPERTIES
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
set_source_files_properties(TextField.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(TextArea.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(TabBar.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(TabButton.qml PROPERTIES
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

qt6_target_qml_files(qtquickcontrols2imaginestyleplugin
    FILES
        ${qml_files}
)
