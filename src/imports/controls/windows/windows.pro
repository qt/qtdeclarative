TARGET = qtquickcontrols2windowsstyleplugin
TARGETPATH = QtQuick/Controls.2/Windows

QML_IMPORT_NAME = QtQuick.Controls.Windows
QML_IMPORT_MAJOR_VERSION = 6

QT += qml quick quickcontrols2 quicktemplates2
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

include(windows.pri)

OTHER_FILES += \
    qmldir \
    $$QML_FILES

SOURCES += \
    $$PWD/qtquickcontrols2windowsstyleplugin.cpp

CONFIG += no_cxx_module install_qml_files builtin_resources qtquickcompiler
CONFIG += qmltypes install_qmltypes

load(qml_plugin)

requires(qtConfig(quickcontrols2-windows))

HEADERS +=
