# Generated from scenegraph.pro.

#####################################################################
## tst_scenegraph Test:
#####################################################################

qt_internal_add_test(tst_scenegraph
    SOURCES
        tst_scenegraph.cpp
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
)

#### Keys ignored in scope 1:.:.:scenegraph.pro:<TRUE>:
# OTHER_FILES = "data/render_OutOfFloatRange.qml" "data/simple.qml" "data/render_ImageFiltering.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_scenegraph CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_scenegraph CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
