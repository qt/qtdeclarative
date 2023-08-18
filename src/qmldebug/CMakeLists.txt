# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmldebug.pro.

#####################################################################
## QmlDebugPrivate Module:
#####################################################################

qt_internal_add_module(QmlDebugPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qqmldebugclient.cpp qqmldebugclient_p.h
        qqmldebugclient_p_p.h
        qqmldebugconnection.cpp qqmldebugconnection_p.h
        qqmldebugmessageclient.cpp qqmldebugmessageclient_p.h
        qqmldebugtranslationclient.cpp qqmldebugtranslationclient_p.h
        qqmlenginecontrolclient.cpp qqmlenginecontrolclient_p.h
        qqmlenginecontrolclient_p_p.h
        qqmlenginedebugclient.cpp qqmlenginedebugclient_p.h
        qqmlenginedebugclient_p_p.h
        qqmlinspectorclient.cpp qqmlinspectorclient_p.h
        qqmlinspectorclient_p_p.h
        qqmlpreviewclient.cpp qqmlpreviewclient_p.h
        qqmlpreviewclient_p_p.h
        qqmlprofilerclient.cpp qqmlprofilerclient_p.h
        qqmlprofilerclient_p_p.h
        qqmlprofilerclientdefinitions_p.h
        qqmlprofilerevent.cpp qqmlprofilerevent_p.h
        qqmlprofilereventlocation.cpp qqmlprofilereventlocation_p.h
        qqmlprofilereventreceiver.cpp qqmlprofilereventreceiver_p.h
        qqmlprofilereventtype.cpp qqmlprofilereventtype_p.h
        qqmlprofilertypedevent.cpp qqmlprofilertypedevent_p.h
        qv4debugclient.cpp qv4debugclient_p.h
        qv4debugclient_p_p.h
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
)
