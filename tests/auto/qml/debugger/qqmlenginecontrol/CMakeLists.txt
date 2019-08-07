# Generated from qqmlenginecontrol.pro.

#####################################################################
## tst_qqmlenginecontrol Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmlenginecontrol
    GUI
    SOURCES
        ../../../shared/util.cpp ../../../shared/util.h
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        tst_qqmlenginecontrol.cpp
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

#### Keys ignored in scope 1:.:.:qqmlenginecontrol.pro:<TRUE>:
# CONFIG = "testcase"
# OTHER_FILES = "data/test.qml" "data/exit.qml"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmlenginecontrol.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmlenginecontrol CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlenginecontrol CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
