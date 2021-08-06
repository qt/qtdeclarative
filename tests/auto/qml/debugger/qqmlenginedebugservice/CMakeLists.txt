# Generated from qqmlenginedebugservice.pro.

#####################################################################
## tst_qqmlenginedebugservice Test:
#####################################################################

qt_internal_add_test(tst_qqmlenginedebugservice
    SOURCES
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        tst_qqmlenginedebugservice.cpp
    DEFINES
        QT_QML_DEBUG_NO_WARNING
    INCLUDE_DIRECTORIES
        ../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::QmlDebugPrivate
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickTestUtilsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlenginedebugservice CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qqmlenginedebugservice CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
