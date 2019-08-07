# Generated from qqmldirparser.pro.

#####################################################################
## tst_qqmldirparser Test:
#####################################################################

add_qt_test(tst_qqmldirparser
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmldirparser.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qqmldirparser.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmldirparser.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmldirparser CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmldirparser CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
