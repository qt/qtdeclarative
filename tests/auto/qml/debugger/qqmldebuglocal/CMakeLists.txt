# Generated from qqmldebuglocal.pro.

#####################################################################
## tst_qqmldebuglocal Test:
#####################################################################

add_qt_test(tst_qqmldebuglocal
    GUI
    SOURCES
        ../../../shared/util.cpp ../../../shared/util.h
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        ../shared/qqmldebugtestservice.cpp ../shared/qqmldebugtestservice.h
        tst_qqmldebuglocal.cpp
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
)

#### Keys ignored in scope 1:.:.:qqmldebuglocal.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmldebuglocal.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmldebuglocal CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmldebuglocal CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
