# Generated from qmlprofiler.pro.

#####################################################################
## qmlprofiler Tool:
#####################################################################

qt_add_tool(qmlprofiler
    TOOLS_TARGET Qml # special case
    SOURCES
        commandlistener.cpp commandlistener.h
        constants.h
        main.cpp
        qmlprofilerapplication.cpp qmlprofilerapplication.h
        qmlprofilerclient.cpp qmlprofilerclient.h
        qmlprofilerdata.cpp qmlprofilerdata.h
    LIBRARIES
        Qt::QmlDebugPrivate
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::QmlDebug
)

#### Keys ignored in scope 1:.:.:qmlprofiler.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Profiler"
