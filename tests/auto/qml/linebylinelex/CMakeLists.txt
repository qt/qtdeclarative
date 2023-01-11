#####################################################################
## tst_linebylinelex Test:
#####################################################################

# Collect linebyline test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/..
    linebylinelex/data/*)
# Collect qmlformat test data
file(GLOB_RECURSE test_data_glob2
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/..
    qmlformat/data/*)
list(APPEND test_data ${test_data_glob} ${test_data_glob2})

qt_internal_add_test(tst_linebylinelex
    SOURCES
        tst_linebylinelex.cpp
    LIBRARIES
        Qt::Qml
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_linebylinelex CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/"
)

qt_internal_extend_target(tst_linebylinelex CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
)
