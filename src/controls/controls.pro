TARGET = QtLabsControls
MODULE = labscontrols

QT += quick
QT_PRIVATE += core-private gui-private qml-private quick-private labstemplates-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

HEADERS += \
    $$PWD/qtlabscontrolsglobal.h
    $$PWD/qtlabscontrolsglobal_p.h

include(controls.pri)
load(qt_module)
