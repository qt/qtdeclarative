# Generated from qmllint.pro.

#####################################################################
## tst_qmllint Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmllint
    SOURCES
        tst_qmllint.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmllint CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qmllint CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)

if (TARGET qmljsrootgen)
    qt_internal_extend_target(tst_qmllint
        DEFINES
            QT_QMLJSROOTGEN_PRESENT
    )
endif()
