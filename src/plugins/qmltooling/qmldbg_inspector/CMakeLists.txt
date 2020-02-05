# Generated from qmldbg_inspector.pro.

#####################################################################
## QQmlInspectorServiceFactory Plugin:
#####################################################################

qt_add_plugin(QQmlInspectorServiceFactory
    OUTPUT_NAME qmldbg_inspector
    TYPE qmltooling
    SOURCES
        globalinspector.cpp globalinspector.h
        highlight.cpp highlight.h
        inspecttool.cpp inspecttool.h
        qqmlinspectorservice.cpp
        qqmlinspectorservicefactory.h
        qquickwindowinspector.cpp qquickwindowinspector.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_inspector.pro:<TRUE>:
# OTHER_FILES = "qqmlinspectorservice.json"
