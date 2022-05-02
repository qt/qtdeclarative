# Generated from qquicktableview.pro.

#####################################################################
## tst_qquicktableview Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquicktableview
    SOURCES
        testmodel.h
        tst_qquicktableview.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTest
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquicktableview.pro:<TRUE>:
# DISTFILES = <EMPTY>

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquicktableview CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qquicktableview CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
