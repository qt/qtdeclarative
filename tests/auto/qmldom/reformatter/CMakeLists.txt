#####################################################################
## tst_reformatter:
#####################################################################
# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/..
    domdata/reformatter/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_reformatter
    SOURCES
        tst_reformatter.cpp tst_reformatter.h
    DEFINES
        QT_DEPRECATED_WARNINGS
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../domdata\\\"
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::Test
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_reformatter CONDITION ANDROID OR IOS
    DEFINES
        QT_REFORMATTERTEST_DATADIR=\\\":/domdata\\\"
)
