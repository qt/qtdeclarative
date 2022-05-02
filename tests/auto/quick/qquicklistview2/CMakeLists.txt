# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquicklistview2
    SOURCES
        typerolemodel.h typerolemodel.cpp
        tst_qquicklistview2.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTest
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

qt6_add_qml_module(tst_qquicklistview2
    URI Test
    VERSION 1.0
)

qt_internal_extend_target(tst_qquicklistview2 CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qquicklistview2 CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
