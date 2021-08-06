# Generated from qquickrendercontrol.pro.

#####################################################################
## tst_qquickrendercontrol Test:
#####################################################################

qt_internal_add_test(tst_qquickrendercontrol
    SOURCES
        tst_qquickrendercontrol.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
)

#### Keys ignored in scope 1:.:.:qquickrendercontrol.pro:<TRUE>:
# OTHER_FILES = "data/rect.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickrendercontrol CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qquickrendercontrol CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
