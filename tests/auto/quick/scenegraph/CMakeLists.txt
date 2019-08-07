# Generated from scenegraph.pro.

#####################################################################
## tst_scenegraph Test:
#####################################################################

add_qt_test(tst_scenegraph
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        ../shared/geometrytestutil.cpp ../shared/geometrytestutil.h
        ../shared/viewtestutil.cpp ../shared/viewtestutil.h
        ../shared/visualtestutil.cpp ../shared/visualtestutil.h
        tst_scenegraph.cpp
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
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

#### Keys ignored in scope 1:.:.:scenegraph.pro:<TRUE>:
# CONFIG = "testcase"
# OTHER_FILES = "data/render_OutOfFloatRange.qml" "data/simple.qml" "data/render_ImageFiltering.qml"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:scenegraph.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_scenegraph CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_scenegraph CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
