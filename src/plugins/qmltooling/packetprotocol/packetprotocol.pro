TARGET = QtPacketProtocol
QT     = core-private qml-private
CONFIG += static internal_module

HEADERS = \
    qpacketprotocol_p.h

SOURCES = \
    qpacketprotocol.cpp

load(qt_module)
