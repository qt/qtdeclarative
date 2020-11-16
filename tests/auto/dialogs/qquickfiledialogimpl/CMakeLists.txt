# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickfiledialogimpl
    SOURCES
        ../../shared/qtest_quickcontrols.h
        ../../shared/util.cpp ../../shared/util.h
        ../../shared/visualtestutil.cpp ../../shared/visualtestutil.h
        tst_qquickfiledialogimpl.cpp
    DEFINES
        QQC2_IMPORT_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/imports\\\"
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2
        Qt::QuickControls2Private
        Qt::QuickDialogs2Private
        Qt::QuickDialogs2QuickImplPrivate
        Qt::QuickDialogs2UtilsPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::QuickTest
        Qt::TestPrivate
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qquickfiledialogimpl CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qquickfiledialogimpl CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
