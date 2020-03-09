# Generated from qquickiconimage.pro.

#####################################################################
## tst_qquickiconimage Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*.qml)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/icons/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qquickiconimage
    SOURCES
        ../shared/qtest_quickcontrols.h
        ../shared/util.cpp ../shared/util.h
        ../shared/visualtestutil.cpp ../shared/visualtestutil.h
        tst_qquickiconimage.cpp
    DEFINES
        QQC2_IMPORT_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../src/imports\\\"
    LIBRARIES
        Qt::QuickControls2Private
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::TestPrivate
    TESTDATA ${test_data}
)

# Resources:
set(resources_resource_files
    "icons/testtheme/16x16/actions/appointment-new.png"
    "icons/testtheme/22x22/actions/+testselector/appointment-new.png"
    "icons/testtheme/22x22/actions/+testselector/appointment-new@2x.png"
    "icons/testtheme/22x22/actions/appointment-new.png"
    "icons/testtheme/22x22/actions/appointment-new@2x.png"
    "icons/testtheme/22x22/actions/color-test-original.png"
    "icons/testtheme/22x22/actions/color-test-original@2x.png"
    "icons/testtheme/22x22/actions/color-test-tinted.png"
    "icons/testtheme/22x22/actions/color-test-tinted@2x.png"
    "icons/testtheme/22x22@2/actions/+testselector/appointment-new.png"
    "icons/testtheme/22x22@2/actions/appointment-new.png"
    "icons/testtheme/appointment-new.svg"
    "icons/testtheme/index.theme"
)

qt_add_resource(tst_qquickiconimage "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)


## Scopes:
#####################################################################

qt_extend_target(tst_qquickiconimage CONDITION TARGET Qt::Svg
    PUBLIC_LIBRARIES
        Qt::Svg
)

qt_extend_target(tst_qquickiconimage CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qquickiconimage CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
