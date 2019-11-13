# Generated from qquickshadereffect.pro.

#####################################################################
## tst_qquickshadereffect Test:
#####################################################################

add_qt_test(tst_qquickshadereffect
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickshadereffect.cpp
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

extend_target(tst_qquickshadereffect CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquickshadereffect CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
