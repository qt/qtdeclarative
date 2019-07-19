# Generated from imagine.pro.

#####################################################################
## qtquickcontrols2imaginestyleplugin Plugin:
#####################################################################

add_qml_module(qtquickcontrols2imaginestyleplugin
    CPP_PLUGIN
    TARGET_PATH "QtQuick/Controls.2/Imagine"
    URI "QtQuick.Controls.Imagine"
    VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    RESOURCE_PREFIX "/qt-project.org/imports"
    SOURCES
        qquickimageselector.cpp qquickimageselector_p.h
        qquickimaginestyle.cpp qquickimaginestyle_p.h
        qquickimaginetheme.cpp qquickimaginetheme_p.h
        qquickninepatchimage.cpp qquickninepatchimage_p.h
        qtquickcontrols2imaginestyleplugin.cpp
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

set(qml_files
    ApplicationWindow.qml
    BusyIndicator.qml
    Button.qml
    CheckBox.qml
    CheckDelegate.qml
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
    MenuItem.qml
    MenuSeparator.qml
    PageIndicator.qml
    Page.qml
    Pane.qml
    Popup.qml
    ProgressBar.qml
    RadioButton.qml
    RadioDelegate.qml
    RangeSlider.qml
    RoundButton.qml
    ScrollBar.qml
    ScrollIndicator.qml
    Slider.qml
    SpinBox.qml
    SplitView.qml
    StackView.qml
    SwipeDelegate.qml
    SwipeView.qml
    Switch.qml
    SwitchDelegate.qml
    TextField.qml
    TextArea.qml
    TabBar.qml
    TabButton.qml
    ToolBar.qml
    ToolButton.qml
    ToolSeparator.qml
    ToolTip.qml
    Tumbler.qml
)

add_qt_resource(qtquickcontrols2imaginestyleplugin qmake_QtQuick_Controls_2_Imagine
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls.2/Imagine"
    FILES
        ${qml_files}
)

qt_install_qml_files(qtquickcontrols2imaginestyleplugin
    FILES ${qml_files}
)


#### Keys ignored in scope 1:.:.:imagine.pro:<TRUE>:
# CONFIG = "no_cxx_module" "install_qml_files" "builtin_resources" "qtquickcompiler"
# IMPORT_NAME = "QtQuick.Controls.Imagine"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# OTHER_FILES = "qmldir" "$$QML_FILES"
# _LOADED = "qml_plugin"
# qtquickcontrols2imaginestyle.files = "$$files$$PWD/images/*.png" "$$files$$PWD/images/*.webp"
# qtquickcontrols2imaginestyle.prefix = "qt-project.org/imports/QtQuick/Controls.2/Imagine"
