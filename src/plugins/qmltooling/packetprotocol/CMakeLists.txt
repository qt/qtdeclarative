# Generated from packetprotocol.pro.

#####################################################################
## PacketProtocolPrivate Module:
#####################################################################

qt_internal_add_module(PacketProtocolPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qpacket.cpp qpacket_p.h
        qpacketprotocol.cpp qpacketprotocol_p.h
        qversionedpacket_p.h
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)
