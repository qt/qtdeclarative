TARGET = qtquickcontrols2basicstyleplugin
TARGETPATH = QtQuick/Controls/Basic

QML_IMPORT_NAME = QtQuick.Controls.Basic
QML_IMPORT_VERSION = $$QT_VERSION

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

include(basic.pri)

OTHER_FILES += \
    qmldir \
    $$QML_FILES

SOURCES += \
    $$PWD/qtquickcontrols2basicstyleplugin.cpp

RESOURCES += \
    $$PWD/qtquickcontrols2basicstyleplugin.qrc

CONFIG += qmltypes no_cxx_module install_qml_files builtin_resources qtquickcompiler
load(qml_plugin)

requires(qtConfig(quickcontrols2-basic))
