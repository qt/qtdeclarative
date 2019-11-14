# Generated from qmldbg_quickprofiler.pro.

#####################################################################
## qmldbg_quickprofiler Plugin:
#####################################################################

qt_add_plugin(qmldbg_quickprofiler
    TYPE qmltooling
    CLASS_NAME QQuickProfilerAdapterFactory
    SOURCES
        qquickprofileradapter.cpp qquickprofileradapter.h
        qquickprofileradapterfactory.cpp qquickprofileradapterfactory.h
    LIBRARIES
        Qt::CorePrivate
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::PacketProtocol
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:qmldbg_quickprofiler.pro:<TRUE>:
# OTHER_FILES = "qquickprofileradapter.json"
