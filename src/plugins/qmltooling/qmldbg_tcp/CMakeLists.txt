# Generated from qmldbg_tcp.pro.

#####################################################################
## QTcpServerConnectionFactory Plugin:
#####################################################################

qt_add_plugin(QTcpServerConnectionFactory
    OUTPUT_NAME qmldbg_tcp
    TYPE qmltooling
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
