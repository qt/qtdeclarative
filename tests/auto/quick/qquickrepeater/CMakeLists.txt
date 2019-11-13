# Generated from qquickrepeater.pro.

#####################################################################
## tst_qquickrepeater Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquickrepeater
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        ../shared/geometrytestutil.cpp ../shared/geometrytestutil.h
        ../shared/viewtestutil.cpp ../shared/viewtestutil.h
        ../shared/visualtestutil.cpp ../shared/visualtestutil.h
        tst_qquickrepeater.cpp
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
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

extend_target(tst_qquickrepeater CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquickrepeater CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
