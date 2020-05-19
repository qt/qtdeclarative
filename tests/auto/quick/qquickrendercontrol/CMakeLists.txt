# Generated from qquickrendercontrol.pro.

#####################################################################
## tst_qquickrendercontrol Test:
#####################################################################

qt_add_test(tst_qquickrendercontrol
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickrendercontrol.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
        Qt::QmlPrivate
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:qquickrendercontrol.pro:<TRUE>:
# OTHER_FILES = "data/rect.qml"

## Scopes:
#####################################################################

qt_extend_target(tst_qquickrendercontrol CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qquickrendercontrol CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
