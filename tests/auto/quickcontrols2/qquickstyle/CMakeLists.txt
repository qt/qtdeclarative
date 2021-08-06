# Generated from qquickstyle.pro.

#####################################################################
## tst_qquickstyle Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickstyle
    SOURCES
        tst_qquickstyle.cpp
    DEFINES
        QQC2_IMPORT_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/quickcontrols2\\\"
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
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

# Resources:
set(qmake_custom_resource_files
    "data/Custom/Label.qml"
    "data/Custom/qmldir"
)

qt_internal_add_resource(tst_qquickstyle "qmake_custom"
    PREFIX
        "/"
    FILES
        ${qmake_custom_resource_files}
)


#### Keys ignored in scope 1:.:.:qquickstyle.pro:<TRUE>:
# OTHER_FILES = "data/CmdLineArgStyle/Control.qml" "data/CmdLineArgStyle/qmldir" "data/EnvVarStyle/Control.qml" "data/EnvVarStyle/qmldir" "data/EnvVarFallbackStyle/Control.qml" "data/EnvVarFallbackStyle/qmldir"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickstyle CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qquickstyle CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
