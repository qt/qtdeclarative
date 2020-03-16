# Generated from qqmldebuggingenabler.pro.

#####################################################################
## tst_qqmldebuggingenabler Test:
#####################################################################

qt_add_test(tst_qqmldebuggingenabler
    SOURCES
        ../../../../shared/util.cpp ../../../../shared/util.h
        ../../shared/debugutil.cpp ../../shared/debugutil_p.h
        ../../shared/qqmldebugprocess.cpp ../../shared/qqmldebugprocess_p.h
        tst_qqmldebuggingenabler.cpp
    INCLUDE_DIRECTORIES
        ../../../../shared
        ../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlDebugPrivate
)

#### Keys ignored in scope 1:.:.:qqmldebuggingenabler.pro:<TRUE>:
# OTHER_FILES = "data/test.qml"

## Scopes:
#####################################################################

qt_extend_target(tst_qqmldebuggingenabler CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qqmldebuggingenabler CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
