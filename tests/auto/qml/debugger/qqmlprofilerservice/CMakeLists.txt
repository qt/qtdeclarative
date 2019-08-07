# Generated from qqmlprofilerservice.pro.

#####################################################################
## tst_qqmlprofilerservice Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmlprofilerservice
    GUI
    SOURCES
        ../../../shared/util.cpp ../../../shared/util.h
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        tst_qqmlprofilerservice.cpp
    INCLUDE_DIRECTORIES
        ../../../shared
        ../shared
    LIBRARIES
        Qt::GuiPrivate
        Qt::QmlDebugPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::QmlDebug
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmlprofilerservice.pro:<TRUE>:
# CONFIG = "testcase"
# OTHER_FILES = "data/pixmapCacheTest.qml" "data/controlFromJS.qml" "data/test.qml" "data/exit.qml" "data/scenegraphTest.qml" "data/TestImage_2x2.png" "data/signalSourceLocation.qml" "data/javascript.qml" "data/timer.qml" "data/qstr.qml" "data/memory.qml" "data/batchOverflow.qml"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmlprofilerservice.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmlprofilerservice CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlprofilerservice CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
