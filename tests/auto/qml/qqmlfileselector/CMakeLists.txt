# Generated from qqmlfileselector.pro.

#####################################################################
## tst_qqmlfileselector Test:
#####################################################################

add_qt_test(tst_qqmlfileselector
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlfileselector.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qqmlfileselector.pro:<TRUE>:
# CONFIG = "testcase"
# TESTDATA = "data/*"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmlfileselector.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmlfileselector CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlfileselector CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"/data\\\"
)
