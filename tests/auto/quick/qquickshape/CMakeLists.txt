# Generated from qquickshape.pro.

#####################################################################
## tst_qquickshape Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquickshape
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        ../shared/geometrytestutil.cpp ../shared/geometrytestutil.h
        ../shared/viewtestutil.cpp ../shared/viewtestutil.h
        ../shared/visualtestutil.cpp ../shared/visualtestutil.h
        tst_qquickshape.cpp
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickShapesPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickShapes
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickshape.pro:<TRUE>:
# DISTFILES = "data/*"

## Scopes:
#####################################################################

extend_target(tst_qquickshape CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)

extend_target(tst_qquickshape CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquickshape CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
