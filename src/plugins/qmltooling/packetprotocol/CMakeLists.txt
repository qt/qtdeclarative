# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
