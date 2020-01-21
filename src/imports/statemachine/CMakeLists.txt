# Generated from statemachine.pro.

#####################################################################
## qtqmlstatemachine Plugin:
#####################################################################

qt_add_qml_module(qtqmlstatemachine
    URI "QtQml.StateMachine"
    VERSION "1.15"
    CLASSNAME QtQmlStateMachinePlugin
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
    SOURCES
        childrenprivate.h
        finalstate.cpp finalstate.h
        plugin.cpp
        signaltransition.cpp signaltransition.h
        state.cpp state.h
        statemachine.cpp statemachine.h
        statemachineforeign.h
        timeouttransition.cpp timeouttransition.h
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:statemachine.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.15"
# TARGETPATH = "QtQml/StateMachine"
