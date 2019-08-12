# Generated from qmldbg_tcp.pro.

#####################################################################
## qmldbg_tcp Plugin:
#####################################################################

add_qt_plugin(qmldbg_tcp
    TYPE qmltooling
    CLASS_NAME QTcpServerConnectionFactory
    SOURCES
        qtcpserverconnection.cpp
        qtcpserverconnectionfactory.h
    LIBRARIES
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qmldbg_tcp.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qtcpserverconnection.json"
# PLUGIN_CLASS_NAME = "QTcpServerConnectionFactory"
# PLUGIN_TYPE = "qmltooling"
# _LOADED = "qt_plugin"
