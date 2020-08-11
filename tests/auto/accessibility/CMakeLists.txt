# Generated from accessibility.pro.

if(NOT QT_FEATURE_accessibility)
    return()
endif()

#####################################################################
## tst_accessibility Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_accessibility
    SOURCES
        ../shared/qtest_quickcontrols.h
        ../shared/util.cpp ../shared/util.h
        ../shared/visualtestutil.cpp ../shared/visualtestutil.h
        tst_accessibility.cpp
    DEFINES
        QQC2_IMPORT_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../src/imports\\\"
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2
        Qt::QuickControls2Private
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::TestPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:accessibility.pro:<TRUE>:
# OTHER_FILES = "data/*.qml"
# _REQUIREMENTS = "qtConfig(accessibility)"

## Scopes:
#####################################################################

qt_extend_target(tst_accessibility CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_accessibility CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
