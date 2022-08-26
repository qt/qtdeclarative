# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmlprofiler.pro.

#####################################################################
## qmlprofiler Tool:
#####################################################################

qt_get_tool_target_name(target_name qmlprofiler)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Profiler"
    TOOLS_TARGET Qml # special case
    SOURCES
        commandlistener.cpp commandlistener.h
        constants.h
        main.cpp
        qmlprofilerapplication.cpp qmlprofilerapplication.h
        qmlprofilerclient.cpp qmlprofilerclient.h
        qmlprofilerdata.cpp qmlprofilerdata.h
    LIBRARIES
        Qt::Network
        Qt::QmlDebugPrivate
)
qt_internal_return_unless_building_tools()

#### Keys ignored in scope 1:.:.:qmlprofiler.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Profiler"
