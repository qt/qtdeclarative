# Generated from qquickscreen.pro.

#####################################################################
## tst_qquickscreen Test:
#####################################################################

add_qt_test(tst_qquickscreen
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickscreen.cpp
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

## Scopes:
#####################################################################

extend_target(tst_qquickscreen CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquickscreen CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
