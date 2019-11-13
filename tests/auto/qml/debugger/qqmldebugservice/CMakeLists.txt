# Generated from qqmldebugservice.pro.

#####################################################################
## tst_qqmldebugservice Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmldebugservice
    SOURCES
        ../../../shared/util.cpp ../../../shared/util.h
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        ../shared/qqmldebugtestservice.cpp ../shared/qqmldebugtestservice.h
        tst_qqmldebugservice.cpp
    DEFINES
        QT_QML_DEBUG_NO_WARNING
    INCLUDE_DIRECTORIES
        ../../../shared
        ../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlDebugPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QmlDebug
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmldebugservice.pro:<TRUE>:
# OTHER_FILES = "data/test.qml"

## Scopes:
#####################################################################

extend_target(tst_qqmldebugservice CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmldebugservice CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
