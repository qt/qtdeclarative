TARGET = QtLabsTemplates
MODULE = labstemplates
CONFIG += internal_module

QT += quick
QT_PRIVATE += core-private gui-private qml-private quick-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

HEADERS += \
    $$PWD/qtlabstemplatesglobal_p.h

include(templates.pri)
load(qt_module)
