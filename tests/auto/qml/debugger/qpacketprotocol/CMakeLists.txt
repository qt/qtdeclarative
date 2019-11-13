# Generated from qpacketprotocol.pro.

#####################################################################
## tst_qpacketprotocol Test:
#####################################################################

add_qt_test(tst_qpacketprotocol
    SOURCES
        ../../../shared/util.cpp ../../../shared/util.h
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        tst_qpacketprotocol.cpp
    INCLUDE_DIRECTORIES
        ../../../shared
        ../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlDebugPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::QmlDebug
)

## Scopes:
#####################################################################

extend_target(tst_qpacketprotocol CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qpacketprotocol CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
