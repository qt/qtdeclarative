# Generated from qqmlenginedebugservice.pro.

#####################################################################
## tst_qqmlenginedebugservice Test:
#####################################################################

qt_add_test(tst_qqmlenginedebugservice
    SOURCES
        ../../../shared/util.cpp ../../../shared/util.h
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        tst_qqmlenginedebugservice.cpp
    DEFINES
        QT_QML_DEBUG_NO_WARNING
    INCLUDE_DIRECTORIES
        ../../../shared
        ../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::QmlDebugPrivate
        Qt::QmlPrivate
        Qt::Quick
)

## Scopes:
#####################################################################

qt_extend_target(tst_qqmlenginedebugservice CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qqmlenginedebugservice CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
