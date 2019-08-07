# Generated from qqmlengine.pro.

#####################################################################
## tst_qqmlengine Test:
#####################################################################

add_qt_test(tst_qqmlengine
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlengine.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Qml
)

# Resources:
add_qt_resource(tst_qqmlengine "qmake_immediate"
    PREFIX
        "/"
    FILES
        data/qrcurls.js
        data/qrcurls.qml
)


#### Keys ignored in scope 1:.:.:qqmlengine.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmlengine.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmlengine CONDITION boot2qt
    DEFINES
        SKIP_GCCORRUPTION_TEST
)

extend_target(tst_qqmlengine CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlengine CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
