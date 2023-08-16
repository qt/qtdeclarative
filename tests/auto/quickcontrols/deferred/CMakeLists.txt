if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickdeferred LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickdeferred
    SOURCES
        tst_qquickdeferred.cpp
    LIBRARIES
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickTemplates2Private
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qquickdeferred CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickdeferred CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
