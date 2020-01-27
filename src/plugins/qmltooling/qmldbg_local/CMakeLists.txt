# Generated from qmldbg_local.pro.

#####################################################################
## QLocalClientConnectionFactory Plugin:
#####################################################################

qt_add_plugin(QLocalClientConnectionFactory
    OUTPUT_NAME qmldbg_local
    TYPE qmltooling
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
