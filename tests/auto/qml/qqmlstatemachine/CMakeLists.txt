# Generated from qqmlstatemachine.pro.

#####################################################################
## tst_qqmlstatemachine Test:
#####################################################################

qt_add_test(tst_qqmlstatemachine
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlstatemachine.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)

## Scopes:
#####################################################################

qt_extend_target(tst_qqmlstatemachine CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qqmlstatemachine CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
