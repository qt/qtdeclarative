#####################################################################
## tst_qqmltreemodeltotablemodel Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmltreemodeltotablemodel
    SOURCES
        tst_qqmltreemodeltotablemodel.cpp
        testmodel.h testmodel.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmltreemodeltotablemodel CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qqmltreemodeltotablemodel CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
