# Generated from fusion.pro.

#####################################################################
## qtquickcontrols2fusionstyleplugin Plugin:
#####################################################################

add_qml_module(qtquickcontrols2fusionstyleplugin
    CPP_PLUGIN
    TARGET_PATH "QtQuick/Controls.2/Fusion"
    URI "QtQuick.Controls.Fusion"
    VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    SOURCES
        qquickfusionbusyindicator.cpp qquickfusionbusyindicator_p.h
        qquickfusiondial.cpp qquickfusiondial_p.h
        qquickfusionknob.cpp qquickfusionknob_p.h
        qquickfusionstyle.cpp qquickfusionstyle_p.h
        qquickfusiontheme.cpp qquickfusiontheme_p.h
        qtquickcontrols2fusionstyleplugin.cpp
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
add_qt_resource(qtquickcontrols2fusionstyleplugin "qtquickcontrols2fusionstyle"
    PREFIX
        "qt-project.org/imports/QtQuick/Controls.2/Fusion"
    FILES
        images/arrow.png
        images/arrow@2x.png
        images/arrow@3x.png
        images/arrow@4x.png
        images/checkmark.png
        images/checkmark@2x.png
        images/checkmark@3x.png
        images/checkmark@4x.png
        images/progressmask.png
        images/progressmask@2x.png
        images/progressmask@3x.png
        images/progressmask@4x.png
)


set(qml_files
    ApplicationWindow.qml
    BusyIndicator.qml
    Button.qml
    ButtonPanel.qml
    CheckBox.qml
    CheckDelegate.qml
    CheckIndicator.qml
    ComboBox.qml
    DelayButton.qml
    Dial.qml
    Dialog.qml
    DialogButtonBox.qml
    Drawer.qml
    Frame.qml
    GroupBox.qml
    ItemDelegate.qml
    Label.qml
    Menu.qml
    MenuBar.qml
    MenuBarItem.qml
    MenuItem.qml
    MenuSeparator.qml
    Page.qml
    PageIndicator.qml
    Pane.qml
    Popup.qml
    ProgressBar.qml
    RadioButton.qml
    RadioDelegate.qml
    RadioIndicator.qml
    RangeSlider.qml
    RoundButton.qml
    ScrollBar.qml
    ScrollIndicator.qml
    Slider.qml
    SliderGroove.qml
    SliderHandle.qml
    SpinBox.qml
    SplitView.qml
    SwipeDelegate.qml
    SwitchDelegate.qml
    SwitchIndicator.qml
    Switch.qml
    TabBar.qml
    TabButton.qml
    TextArea.qml
    TextField.qml
    ToolBar.qml
    ToolButton.qml
    ToolSeparator.qml
    ToolTip.qml
    Tumbler.qml
)

add_qt_resource(qtquickcontrols2fusionstyleplugin qmake_QtQuick_Controls_2_Fusion
    FILES
        ${qml_files}
)

qt_install_qml_files(qtquickcontrols2fusionstyleplugin
    FILES ${qml_files}
)


#### Keys ignored in scope 1:.:.:fusion.pro:<TRUE>:
# CONFIG = "no_cxx_module" "install_qml_files" "builtin_resources" "qtquickcompiler"
# IMPORT_NAME = "QtQuick.Controls.Fusion"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# OTHER_FILES = "qmldir" "$$QML_FILES"
# _LOADED = "qml_plugin"
