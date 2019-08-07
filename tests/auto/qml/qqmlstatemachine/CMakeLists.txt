# Generated from qqmlstatemachine.pro.

#####################################################################
## tst_qqmlstatemachine Test:
#####################################################################

add_qt_test(tst_qqmlstatemachine
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlstatemachine.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:qqmlstatemachine.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmlstatemachine.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmlstatemachine CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlstatemachine CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
