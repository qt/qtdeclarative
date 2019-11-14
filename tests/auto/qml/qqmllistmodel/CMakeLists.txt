# Generated from qqmllistmodel.pro.

#####################################################################
## tst_qqmllistmodel Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmllistmodel
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmllistmodel.cpp
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

extend_target(tst_qqmllistmodel CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmllistmodel CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
