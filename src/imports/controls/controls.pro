TARGET  = qtquickcontrols2plugin
TARGETPATH = QtQuick/Controls.2
IMPORT_VERSION = 2.0

QT += qml quick
QT += core-private gui-private qml-private quick-private quickcontrols-private

OTHER_FILES += \
    qmldir

QML_FILES = \
    ApplicationWindow.qml \
    BusyIndicator.qml \
    Button.qml \
    CheckBox.qml \
    Frame.qml \
    GroupBox.qml \
    Label.qml \
    PageIndicator.qml \
    ProgressBar.qml \
    RadioButton.qml \
    ScrollBar.qml \
    ScrollIndicator.qml \
    Slider.qml \
    SpinBox.qml \
    StackView.js \
    StackView.qml \
    StackViewDelegate.qml \
    StackViewTransition.qml \
    Switch.qml \
    TabBar.qml \
    TabButton.qml \
    TabView.qml \
    TextArea.qml \
    TextField.qml \
    ToggleButton.qml \
    ToolBar.qml \
    ToolButton.qml

SOURCES += \
    $$PWD/qtquickcontrols2plugin.cpp

RESOURCES += \
    $$PWD/qtquickcontrols2plugin.qrc

CONFIG += no_cxx_module
load(qml_plugin)
