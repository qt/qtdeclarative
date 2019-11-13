# Generated from quicktestmainwithsetup.pro.

#####################################################################
## tst_quicktestmainwithsetup Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*.qml)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_quicktestmainwithsetup
    QMLTEST
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_quicktestmainwithsetup.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

extend_target(tst_quicktestmainwithsetup CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_quicktestmainwithsetup CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
