# Generated from packetprotocol.pro.

#####################################################################
## PacketProtocol Module:
#####################################################################

qt_internal_add_module(PacketProtocol
    STATIC
    INTERNAL_MODULE
    SOURCES
        qpacket.cpp qpacket_p.h
        qpacketprotocol.cpp qpacketprotocol_p.h
        qversionedpacket_p.h
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)
