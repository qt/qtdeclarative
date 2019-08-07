# Generated from qqmlimport.pro.

#####################################################################
## tst_qqmlimport Test:
#####################################################################

add_qt_test(tst_qqmlimport
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlimport.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:qqmlimport.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmlimport.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmlimport CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlimport CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
