# Generated from qquickwindow.pro.

#####################################################################
## tst_qquickwindow Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickwindow
    SOURCES
        tst_qquickwindow.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickwindow.pro:<TRUE>:
# OTHER_FILES = "data/active.qml" "data/AnimationsWhileHidden.qml" "data/Headless.qml" "data/showHideAnimate.qml" "data/windoworder.qml" "data/grabContentItemToImage.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickwindow CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qquickwindow CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
