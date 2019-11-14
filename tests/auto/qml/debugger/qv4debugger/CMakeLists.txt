# Generated from qv4debugger.pro.

#####################################################################
## tst_qv4debugger Test:
#####################################################################

qt_add_test(tst_qv4debugger
    SOURCES
        ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4datacollector.cpp ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4datacollector.h
        ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4debugger.cpp ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4debugger.h
        ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4debugjob.cpp ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4debugjob.h
        tst_qv4debugger.cpp
    INCLUDE_DIRECTORIES
        ../../../../../src/plugins/qmltooling/qmldbg_debugger
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Qml
)

## Scopes:
#####################################################################
