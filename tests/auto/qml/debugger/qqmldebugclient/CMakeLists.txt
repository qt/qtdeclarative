# Generated from qqmldebugclient.pro.

#####################################################################
## tst_qqmldebugclient Test:
#####################################################################

qt_add_test(tst_qqmldebugclient
    SOURCES
        ../../../shared/util.cpp ../../../shared/util.h
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        ../shared/qqmldebugtestservice.cpp ../shared/qqmldebugtestservice.h
        tst_qqmldebugclient.cpp
    DEFINES
        QT_QML_DEBUG
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

## Scopes:
#####################################################################

qt_extend_target(tst_qqmldebugclient CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qqmldebugclient CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
