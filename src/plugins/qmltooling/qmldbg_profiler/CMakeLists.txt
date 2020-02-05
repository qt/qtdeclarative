# Generated from qmldbg_profiler.pro.

#####################################################################
## QQmlProfilerServiceFactory Plugin:
#####################################################################

qt_add_plugin(QQmlProfilerServiceFactory
    OUTPUT_NAME qmldbg_profiler
    TYPE qmltooling
    SOURCES
        qqmlenginecontrolservice.cpp qqmlenginecontrolservice.h
        qqmlprofileradapter.cpp qqmlprofileradapter.h
        qqmlprofilerservice.cpp qqmlprofilerservice.h
        qqmlprofilerservicefactory.cpp qqmlprofilerservicefactory.h
        qv4profileradapter.cpp qv4profileradapter.h
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_profiler.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qqmlprofilerservice.json"
