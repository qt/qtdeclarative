#####################################################################
## tst_linebylinelex Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_linebylinelex LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect linebyline test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
# Collect qmlformat test data
file(GLOB_RECURSE test_data_glob2
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ../qmlformat/data/*)
list(APPEND test_data ${test_data_glob} ${test_data_glob2})

qt_internal_add_test(tst_linebylinelex
    SOURCES
        tst_linebylinelex.cpp
    LIBRARIES
        Qt::Qml
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
    BUILTIN_TESTDATA
)
