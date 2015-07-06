TARGET  = qtquickcontrols2plugin
TARGETPATH = QtQuick/Controls.2
IMPORT_VERSION = 2.0

QT += qml quick
QT += core-private gui-private qml-private quick-private quickcontrols2-private

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
    StackView.qml \
    Switch.qml \
    TabBar.qml \
    TabButton.qml \
    TextArea.qml \
    TextField.qml \
    ToggleButton.qml \
    ToolBar.qml \
    ToolButton.qml

HEADERS += \
    $$PWD/qquicktheme_p.h \
    $$PWD/qquickthemedata_p.h

SOURCES += \
    $$PWD/qquicktheme.cpp \
    $$PWD/qquickthemedata.cpp \
    $$PWD/qtquickcontrols2plugin.cpp

RESOURCES += \
    $$PWD/qtquickcontrols2plugin.qrc

OTHER_FILES += \
    $$PWD/theme.json

include(designer/designer.pri)

CONFIG += no_cxx_module
load(qml_plugin)
