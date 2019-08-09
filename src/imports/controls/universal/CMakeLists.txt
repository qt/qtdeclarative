# Generated from universal.pro.

#####################################################################
## qtquickcontrols2universalstyleplugin Plugin:
#####################################################################

add_qml_module(qtquickcontrols2universalstyleplugin
    CPP_PLUGIN
    TARGET_PATH "QtQuick/Controls.2/Universal"
    URI "QtQuick.Controls.Universal"
    VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    SOURCES
        qquickuniversalbusyindicator.cpp qquickuniversalbusyindicator_p.h
        qquickuniversalfocusrectangle.cpp qquickuniversalfocusrectangle_p.h
        qquickuniversalprogressbar.cpp qquickuniversalprogressbar_p.h
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
add_qt_resource(qtquickcontrols2universalstyleplugin "qtquickcontrols2universalstyleplugin"
    PREFIX
        "qt-project.org/imports/QtQuick/Controls.2/Universal"
    FILES
        images/checkmark.png
        images/checkmark@2x.png
        images/checkmark@3x.png
        images/checkmark@4x.png
        images/downarrow.png
        images/downarrow@2x.png
        images/downarrow@3x.png
        images/downarrow@4x.png
        images/leftarrow.png
        images/leftarrow@2x.png
        images/leftarrow@3x.png
        images/leftarrow@4x.png
        images/rightarrow.png
        images/rightarrow@2x.png
        images/rightarrow@3x.png
        images/rightarrow@4x.png
)


set(qml_files
    ApplicationWindow.qml
    BusyIndicator.qml
    Button.qml
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
    SpinBox.qml
    SplitView.qml
    StackView.qml
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

add_qt_resource(qtquickcontrols2universalstyleplugin qmake_QtQuick_Controls_2_Universal
    FILES
        ${qml_files}
)

qt_install_qml_files(qtquickcontrols2universalstyleplugin
    FILES ${qml_files}
)


#### Keys ignored in scope 1:.:.:universal.pro:<TRUE>:
# CONFIG = "no_cxx_module" "install_qml_files" "builtin_resources" "qtquickcompiler"
# IMPORT_NAME = "QtQuick.Controls.Universal"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# OTHER_FILES = "qmldir" "$$QML_FILES"
# _LOADED = "qml_plugin"
