# Generated from tst_qqmlenginecleanup.pro.

#####################################################################
## tst_qqmlenginecleanup Test:
#####################################################################

qt_add_test(tst_qqmlenginecleanup
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlenginecleanup.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QmlPrivate
)

## Scopes:
#####################################################################

qt_extend_target(tst_qqmlenginecleanup CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qqmlenginecleanup CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
