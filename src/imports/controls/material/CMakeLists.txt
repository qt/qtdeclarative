# Generated from material.pro.

#####################################################################
## qtquickcontrols2materialstyleplugin Plugin:
#####################################################################

add_qml_module(qtquickcontrols2materialstyleplugin
    TARGET_PATH "QtQuick/Controls.2/Material"
    URI "QtQuick.Controls.Material"
    VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    CLASSNAME QtQuickControls2MaterialStylePlugin
    DEPENDENCIES
        QtQuick.Controls/2.5
    SKIP_TYPE_REGISTRATION
    SOURCES
        qquickmaterialbusyindicator.cpp qquickmaterialbusyindicator_p.h
        qquickmaterialprogressbar.cpp qquickmaterialprogressbar_p.h
        qquickmaterialripple.cpp qquickmaterialripple_p.h
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

add_qt_resource(qtquickcontrols2materialstyleplugin "qtquickcontrols2materialstyleplugin"
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls.2/Material"
    FILES
        ${qtquickcontrols2materialstyleplugin_resource_files}
)


#### Keys ignored in scope 1:.:.:material.pro:<TRUE>:
# IMPORT_NAME = "QtQuick.Controls.Material"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# OTHER_FILES = "qmldir" "$$QML_FILES"
# TARGETPATH = "QtQuick/Controls.2/Material"
# _LOADED = "qml_plugin"
# _REQUIREMENTS = "qtConfig(quickcontrols2-material)"

set(qml_files
    "ApplicationWindow.qml"
    "BoxShadow.qml"
    "BusyIndicator.qml"
    "Button.qml"
    "CheckBox.qml"
    "CheckDelegate.qml"
    "CheckIndicator.qml"
    "ComboBox.qml"
    "CursorDelegate.qml"
    "DelayButton.qml"
    "Dial.qml"
    "Dialog.qml"
    "DialogButtonBox.qml"
    "Drawer.qml"
    "ElevationEffect.qml"
    "Frame.qml"
    "GroupBox.qml"
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
    "RadioIndicator.qml"
    "RangeSlider.qml"
    "RoundButton.qml"
    "RectangularGlow.qml"
    "ScrollBar.qml"
    "ScrollIndicator.qml"
    "Slider.qml"
    "SliderHandle.qml"
    "SpinBox.qml"
    "SplitView.qml"
    "StackView.qml"
    "SwipeDelegate.qml"
    "SwipeView.qml"
    "Switch.qml"
    "SwitchDelegate.qml"
    "SwitchIndicator.qml"
    "TabBar.qml"
    "TabButton.qml"
    "TextArea.qml"
    "TextField.qml"
    "ToolBar.qml"
    "ToolButton.qml"
    "ToolSeparator.qml"
    "ToolTip.qml"
    "Tumbler.qml"
)

qt6_target_qml_files(qtquickcontrols2materialstyleplugin
    FILES
        ${qml_files}
)
