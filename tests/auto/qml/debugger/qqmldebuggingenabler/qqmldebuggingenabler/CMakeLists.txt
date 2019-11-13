# Generated from qqmldebuggingenabler.pro.

#####################################################################
## tst_qqmldebuggingenabler Test:
#####################################################################

add_qt_test(tst_qqmldebuggingenabler
    SOURCES
        ../../../../shared/util.cpp ../../../../shared/util.h
        ../../shared/debugutil.cpp ../../shared/debugutil_p.h
        ../../shared/qqmldebugprocess.cpp ../../shared/qqmldebugprocess_p.h
        tst_qqmldebuggingenabler.cpp
    INCLUDE_DIRECTORIES
        ../../../../shared
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlDebugPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QmlDebug
)

#### Keys ignored in scope 1:.:.:qqmldebuggingenabler.pro:<TRUE>:
# OTHER_FILES = "data/test.qml"

## Scopes:
#####################################################################

extend_target(tst_qqmldebuggingenabler CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmldebuggingenabler CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
