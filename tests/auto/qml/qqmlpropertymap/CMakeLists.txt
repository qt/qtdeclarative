# Generated from qqmlpropertymap.pro.

#####################################################################
## tst_qqmlpropertymap Test:
#####################################################################

add_qt_test(tst_qqmlpropertymap
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlpropertymap.cpp
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

#### Keys ignored in scope 1:.:.:qqmlpropertymap.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmlpropertymap.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmlpropertymap CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlpropertymap CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
