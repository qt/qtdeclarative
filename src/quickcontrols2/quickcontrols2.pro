TARGET = QtQuickControls2
MODULE = quickcontrols2

QT += quick
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

HEADERS += \
    $$PWD/qtquickcontrols2global.h \
    $$PWD/qtquickcontrols2global_p.h

include(quickcontrols2.pri)

QMLTYPES_FILENAME = plugins.qmltypes
QMLTYPES_INSTALL_DIR = $$[QT_INSTALL_QML]/QtQuick/Controls.2
QML_IMPORT_NAME = QtQuick.Controls
QML_IMPORT_VERSION = 2.15
CONFIG += qmltypes install_qmltypes install_metatypes

load(qt_module)
