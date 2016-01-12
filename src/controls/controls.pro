TARGET = QtLabsControls
MODULE = labscontrols
CONFIG += static internal_module

QT += quick
QT_PRIVATE += core-private gui-private qml-private quick-private labstemplates-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

include(controls.pri)
load(qt_module)
