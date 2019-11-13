# Generated from qmldbg_local.pro.

#####################################################################
## qmldbg_local Plugin:
#####################################################################

add_qt_plugin(qmldbg_local
    TYPE qmltooling
    CLASS_NAME QLocalClientConnectionFactory
    SOURCES
        qlocalclientconnection.cpp
        qlocalclientconnectionfactory.h
    LIBRARIES
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qmldbg_local.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qlocalclientconnection.json"
