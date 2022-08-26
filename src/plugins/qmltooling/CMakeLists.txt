# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmltooling.pro.

add_subdirectory(packetprotocol)
add_subdirectory(qmldbg_native)
add_subdirectory(qmldbg_messages)
add_subdirectory(qmldbg_profiler)
add_subdirectory(qmldbg_debugger)
add_subdirectory(qmldbg_nativedebugger)
if(QT_FEATURE_thread)
    add_subdirectory(qmldbg_server)
endif()
if(QT_FEATURE_qml_network)
    add_subdirectory(qmldbg_tcp)
endif()
if(QT_FEATURE_localserver AND QT_FEATURE_qml_network)
    add_subdirectory(qmldbg_local)
endif()
if(TARGET Qt::Quick)
    add_subdirectory(qmldbg_inspector)
    add_subdirectory(qmldbg_quickprofiler)
endif()
if(QT_FEATURE_qml_network AND QT_FEATURE_settings AND TARGET Qt::Quick)
    add_subdirectory(qmldbg_preview)
endif()
