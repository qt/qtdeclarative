# Generated from packetprotocol.pro.

#####################################################################
## PacketProtocol Module:
#####################################################################

add_qt_module(PacketProtocol
    STATIC
    INTERNAL_MODULE
    SOURCES
        qpacket.cpp qpacket_p.h
        qpacketprotocol.cpp qpacketprotocol_p.h
        qversionedpacket_p.h
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 1:.:.:packetprotocol.pro:<TRUE>:
# _LOADED = "qt_module"
