TARGET = qtquickcontrols2macosstyleplugin
TARGETPATH = QtQuick/Controls.2/macOS

QML_IMPORT_NAME = QtQuick.Controls.macOS
QML_IMPORT_MAJOR_VERSION = 2

QT += qml quick quickcontrols2 quicktemplates2
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

include(macos.pri)

OTHER_FILES += \
    qmldir \
    $$QML_FILES

SOURCES += \
    $$PWD/qtquickcontrols2macosstyleplugin.cpp

CONFIG += no_cxx_module install_qml_files builtin_resources qtquickcompiler
CONFIG += qmltypes install_qmltypes

load(qml_plugin)

# TODO: add config flag
#requires(qtConfig(quickcontrols2-nativestyle))

HEADERS +=
