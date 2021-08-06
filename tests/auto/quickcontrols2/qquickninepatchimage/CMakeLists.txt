# Generated from qquickninepatchimage.pro.

#####################################################################
## tst_qquickninepatchimage Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*.qml)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*.png)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickninepatchimage
    SOURCES
        tst_qquickninepatchimage.cpp
    DEFINES
        QQC2_IMPORT_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/quickcontrols2\\\"
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickControls2Private
        Qt::QuickControlsTestUtilsPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::QuickTest
        Qt::QuickTestUtilsPrivate
        Qt::TestPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickninepatchimage CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qquickninepatchimage CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
