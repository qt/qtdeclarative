# Generated from qquicklistview.pro.

#####################################################################
## tst_qquicklistview Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquicklistview
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        ../shared/geometrytestutil.cpp ../shared/geometrytestutil.h
        ../shared/viewtestutil.cpp ../shared/viewtestutil.h
        ../shared/visualtestutil.cpp ../shared/visualtestutil.h
        incrementalmodel.cpp incrementalmodel.h
        proxytestinnermodel.cpp proxytestinnermodel.h
        randomsortmodel.cpp randomsortmodel.h
        tst_qquicklistview.cpp
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

#### Keys ignored in scope 1:.:.:qquicklistview.pro:<TRUE>:
# CONFIG = "testcase"
# DISTFILES = "data/*"
# testcase.timeout = "900"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qquicklistview.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qquicklistview CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquicklistview CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
