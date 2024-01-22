# Generated from qv4debugger.pro.

#####################################################################
## tst_qv4debugger Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        data/*)
list(APPEND test_data ${test_data_glob})

qt_add_library(testCppTypes STATIC)
qt_autogen_tools_initial_setup(testCppTypes)
target_link_libraries(testCppTypes PRIVATE Qt::Qml Qt::QmlPrivate Qt::Quick)

qt6_add_qml_module(testCppTypes
    VERSION 1.0
    URI TestTypes
    SOURCES
        commontypes.h
)
qt_autogen_tools_initial_setup(testCppTypesplugin)

qt_internal_add_test(tst_qv4debugger
    SOURCES
        ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4datacollector.cpp ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4datacollector.h
        ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4debugger.cpp ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4debugger.h
        ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4debugjob.cpp ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4debugjob.h
        tst_qv4debugger.cpp
    INCLUDE_DIRECTORIES
        ../../../../../src/plugins/qmltooling/qmldbg_debugger
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
        testCppTypesplugin
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qv4debugger CONDITION ANDROID OR IOS
        DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
        )

qt_internal_extend_target(tst_qv4debugger CONDITION NOT ANDROID AND NOT IOS
        DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
        )
