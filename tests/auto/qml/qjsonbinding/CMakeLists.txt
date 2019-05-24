# Generated from qjsonbinding.pro.

#####################################################################
## tst_qjsonbinding Test:
#####################################################################

add_qt_test(tst_qjsonbinding
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qjsonbinding.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qjsonbinding.pro:<TRUE>:
# CONFIG = "testcase"
# TESTDATA = "data/*"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qjsonbinding.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qjsonbinding CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qjsonbinding CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"/data\\\"
)
