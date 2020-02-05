# Generated from qmldbg_quickprofiler.pro.

#####################################################################
## QQuickProfilerAdapterFactory Plugin:
#####################################################################

qt_add_plugin(QQuickProfilerAdapterFactory
    OUTPUT_NAME qmldbg_quickprofiler
    TYPE qmltooling
    SOURCES
        qquickprofileradapter.cpp qquickprofileradapter.h
        qquickprofileradapterfactory.cpp qquickprofileradapterfactory.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_quickprofiler.pro:<TRUE>:
# OTHER_FILES = "qquickprofileradapter.json"
