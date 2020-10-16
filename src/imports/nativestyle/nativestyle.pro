TARGET = qtquickcontrols2nativestyleplugin
TARGETPATH = QtQuick/NativeStyle

QML_IMPORT_NAME = QtQuick.NativeStyle
QML_IMPORT_MAJOR_VERSION = 6
QML_PAST_MAJOR_VERSIONS = 2

QT += qml quick quickcontrols2 quicktemplates2
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

include(items/items.pri)
include(qstyle/qstyle.pri)
include(controls/controls.pri)
include(util/util.pri)

OTHER_FILES += \
    qmldir \
    $$QML_FILES

SOURCES += \
    $$PWD/qtquickcontrols2nativestyleplugin.cpp

CONFIG += no_cxx_module install_qml_files builtin_resources qtquickcompiler
CONFIG += qmltypes install_qmltypes

load(qml_plugin)

requires(qtConfig(quickcontrols2-macos)|qtConfig(quickcontrols2-windows))

HEADERS +=
