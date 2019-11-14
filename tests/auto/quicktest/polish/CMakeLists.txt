# Generated from polish.pro.

#####################################################################
## tst_polish Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*.qml)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_polish
    QMLTEST
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_polish.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_extend_target(tst_polish CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_polish CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
