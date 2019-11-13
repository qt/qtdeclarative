# Generated from qqmlinspector.pro.

#####################################################################
## tst_qqmlinspector Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmlinspector
    SOURCES
        ../../../shared/util.cpp ../../../shared/util.h
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        tst_qqmlinspector.cpp
    INCLUDE_DIRECTORIES
        ../../../shared
        ../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlDebugPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::QmlDebug
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

extend_target(tst_qqmlinspector CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlinspector CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
