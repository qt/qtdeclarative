TARGET = qtquickcontrols2universalstyleplugin
TARGETPATH = QtQuick/Controls/Universal

QML_IMPORT_NAME = QtQuick.Controls.Universal
QML_IMPORT_VERSION = $$QT_VERSION

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private quickcontrols2-private quickcontrols2impl-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

include(universal.pri)

OTHER_FILES += \
    qmldir \
    $$QML_FILES

SOURCES += \
    $$PWD/qtquickcontrols2universalstyleplugin.cpp

RESOURCES += \
    $$PWD/qtquickcontrols2universalstyleplugin.qrc

CONFIG += qmltypes no_cxx_module install_qml_files builtin_resources qtquickcompiler
load(qml_plugin)

requires(qtConfig(quickcontrols2-universal))
