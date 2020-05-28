# Generated from qmldbg_local.pro.

#####################################################################
## QLocalClientConnectionFactory Plugin:
#####################################################################

qt_internal_add_plugin(QLocalClientConnectionFactory
    OUTPUT_NAME qmldbg_local
    TYPE qmltooling
    SOURCES
        qlocalclientconnection.cpp
        qlocalclientconnectionfactory.h
    PUBLIC_LIBRARIES
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_local.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qlocalclientconnection.json"
