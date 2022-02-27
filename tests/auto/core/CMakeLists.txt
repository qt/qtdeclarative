# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/tst_*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_core
    GUI
    QMLTEST
    SOURCES
        tst_core.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_core)
endif()

qt_internal_extend_target(tst_core CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_core CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
