TARGET = qtquickcontrols2defaultstyleplugin
TARGETPATH = QtQuick/Controls/Default

QML_IMPORT_NAME = QtQuick.Controls.Default
QML_IMPORT_VERSION = 2.$$QT_MINOR_VERSION

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

include(default.pri)

OTHER_FILES += \
    qmldir \
    $$QML_FILES

SOURCES += \
    $$PWD/qtquickcontrols2defaultstyleplugin.cpp

RESOURCES += \
    $$PWD/qtquickcontrols2defaultstyleplugin.qrc

CONFIG += qmltypes no_cxx_module install_qml_files builtin_resources qtquickcompiler
load(qml_plugin)

requires(qtConfig(quickcontrols2-default))
