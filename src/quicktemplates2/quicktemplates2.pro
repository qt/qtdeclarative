TARGET = QtQuickTemplates2
MODULE = quicktemplates2

QT += quick
QT_PRIVATE += core-private gui-private qml-private quick-private
qtHaveModule(qmlmodels): QT += qmlmodels-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

HEADERS += \
    $$PWD/qtquicktemplates2global_p.h

include(quicktemplates2.pri)
load(qt_module)

QMLTYPES_FILENAME = plugins.qmltypes
QMLTYPES_INSTALL_DIR = $$[QT_INSTALL_QML]/QtQuick/Templates
QML_IMPORT_NAME = QtQuick.Templates
QML_IMPORT_VERSION = 2.15
CONFIG += qmltypes install_qmltypes install_metatypes
