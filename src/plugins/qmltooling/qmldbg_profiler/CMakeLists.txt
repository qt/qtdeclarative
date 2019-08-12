# Generated from qmldbg_profiler.pro.

#####################################################################
## qmldbg_profiler Plugin:
#####################################################################

add_qt_plugin(qmldbg_profiler
    TYPE qmltooling
    CLASS_NAME QQmlProfilerServiceFactory
    SOURCES
        qqmlenginecontrolservice.cpp qqmlenginecontrolservice.h
        qqmlprofileradapter.cpp qqmlprofileradapter.h
        qqmlprofilerservice.cpp qqmlprofilerservice.h
        qqmlprofilerservicefactory.cpp qqmlprofilerservicefactory.h
        qv4profileradapter.cpp qv4profileradapter.h
    LIBRARIES
        Qt::CorePrivate
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::PacketProtocol
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qmldbg_profiler.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qqmlprofilerservice.json"
# PLUGIN_CLASS_NAME = "QQmlProfilerServiceFactory"
# PLUGIN_TYPE = "qmltooling"
# _LOADED = "qt_plugin"
