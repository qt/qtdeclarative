# Generated from qmldbg_inspector.pro.

#####################################################################
## qmldbg_inspector Plugin:
#####################################################################

add_qt_plugin(qmldbg_inspector
    TYPE qmltooling
    CLASS_NAME QQmlInspectorServiceFactory
    SOURCES
        globalinspector.cpp globalinspector.h
        highlight.cpp highlight.h
        inspecttool.cpp inspecttool.h
        qqmlinspectorservice.cpp
        qqmlinspectorservicefactory.h
        qquickwindowinspector.cpp qquickwindowinspector.h
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
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

#### Keys ignored in scope 1:.:.:qmldbg_inspector.pro:<TRUE>:
# OTHER_FILES = "qqmlinspectorservice.json"
