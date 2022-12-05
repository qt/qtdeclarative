TEMPLATE = app
TARGET = flatstyle

QT += quick quickcontrols2 widgets

SOURCES += main.cpp

RESOURCES += \
    qtquickcontrols2.conf \
    flatstyle.qml \
    MainForm.ui.qml \
    imports/Flat/Button.qml \
    imports/Flat/CheckBox.qml \
    imports/Flat/qmldir \
    imports/Flat/Switch.qml \
    imports/Theme/Theme.qml \
    imports/Theme/qmldir

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = $$PWD/imports

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols/flatstyle
INSTALLS += target
