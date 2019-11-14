# Generated from qqmlitemmodels.pro.

#####################################################################
## tst_qqmlitemmodels Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmlitemmodels
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        qtestmodel.h
        testtypes.h
        tst_qqmlitemmodels.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmlitemmodels.pro:<TRUE>:
# DISTFILES = "data/modelindex.qml" "data/persistentmodelindex.qml" "data/itemselectionrange.qml" "data/modelindexlist.qml" "data/itemselection.qml" "data/modelindexconversion.qml"

## Scopes:
#####################################################################

extend_target(tst_qqmlitemmodels CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlitemmodels CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
