# Generated from qquickpathview.pro.

#####################################################################
## tst_qquickpathview Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquickpathview
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        ../shared/geometrytestutil.cpp ../shared/geometrytestutil.h
        ../shared/viewtestutil.cpp ../shared/viewtestutil.h
        ../shared/visualtestutil.cpp ../shared/visualtestutil.h
        tst_qquickpathview.cpp
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QmlModels
        Qt::Quick
        Qt::QuickTest
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

extend_target(tst_qquickpathview CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)

extend_target(tst_qquickpathview CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquickpathview CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
