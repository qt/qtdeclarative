# Generated from qv4debugger.pro.

#####################################################################
## tst_qv4debugger Test:
#####################################################################

add_qt_test(tst_qv4debugger
    GUI
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

#### Keys ignored in scope 1:.:.:qv4debugger.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qv4debugger.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
