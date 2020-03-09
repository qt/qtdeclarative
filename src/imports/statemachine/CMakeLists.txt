# Generated from statemachine.pro.

#####################################################################
## qtqmlstatemachine Plugin:
#####################################################################

qt_add_qml_module(qtqmlstatemachine
    URI "QtQml.StateMachine"
    VERSION "${CMAKE_PROJECT_VERSION}"
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
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:statemachine.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQml/StateMachine"
