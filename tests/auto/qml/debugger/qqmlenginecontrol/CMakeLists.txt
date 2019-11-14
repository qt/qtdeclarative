# Generated from qqmlenginecontrol.pro.

#####################################################################
## tst_qqmlenginecontrol Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qqmlenginecontrol
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
# OTHER_FILES = "data/test.qml" "data/exit.qml"

## Scopes:
#####################################################################

qt_extend_target(tst_qqmlenginecontrol CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qqmlenginecontrol CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
