# Generated from controls.pro.

#####################################################################
## qtquickcontrols2plugin Plugin:
#####################################################################

add_qml_module(qtquickcontrols2plugin
    CPP_PLUGIN
    TARGET_PATH "QtQuick/Controls.2"
    URI "QtQuick.Controls"
    VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    RESOURCE_PREFIX "/qt-project.org/imports"
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
add_qt_resource(qtquickcontrols2plugin "qtquickcontrols2plugin"
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls.2"
    FILES
        images/arrow-indicator.png
        images/arrow-indicator@2x.png
        images/arrow-indicator@3x.png
        images/arrow-indicator@4x.png
        images/check.png
        images/check@2x.png
        images/check@3x.png
        images/check@4x.png
        images/dial-indicator.png
        images/dial-indicator@2x.png
        images/dial-indicator@3x.png
        images/dial-indicator@4x.png
        images/double-arrow.png
        images/double-arrow@2x.png
        images/double-arrow@3x.png
        images/double-arrow@4x.png
        images/drop-indicator.png
        images/drop-indicator@2x.png
        images/drop-indicator@3x.png
        images/drop-indicator@4x.png
)


set(qml_files
    AbstractButton.qml
    Action.qml
    ActionGroup.qml
    ApplicationWindow.qml
    BusyIndicator.qml
    Button.qml
    ButtonGroup.qml
    CheckBox.qml
    CheckDelegate.qml
    ComboBox.qml
    Container.qml
    Control.qml
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
    RangeSlider.qml
    RoundButton.qml
    ScrollBar.qml
    ScrollIndicator.qml
    ScrollView.qml
    Slider.qml
    SpinBox.qml
    SplitView.qml
    StackView.qml
    SwipeDelegate.qml
    Switch.qml
    SwitchDelegate.qml
    SwipeView.qml
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

add_qt_resource(qtquickcontrols2plugin qmake_QtQuick_Controls_2
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls.2"
    FILES
        ${qml_files}
)

qt_install_qml_files(qtquickcontrols2plugin
    FILES ${qml_files}
)


#### Keys ignored in scope 1:.:.:controls.pro:<TRUE>:
# CONFIG = "no_cxx_module" "install_qml_files" "builtin_resources" "qtquickcompiler"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# OTHER_FILES = "qmldir" "$$QML_FILES" "$$files$$PWD/snippets/*.qml" "$$files$$PWD/src/*.qdoc" "$$files$$PWD/src/calendar/*.qdoc" "$$files$$PWD/src/templates/*.qdoc"
# _LOADED = "qml_plugin"

## Scopes:
#####################################################################
add_qt_docs(
    doc/qtquickcontrols.qdocconf
)

# special case begin
add_subdirectory(fusion)
add_subdirectory(imagine)
add_subdirectory(material)
add_subdirectory(universal)
# special case end
