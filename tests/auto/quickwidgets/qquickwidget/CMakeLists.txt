# Generated from qquickwidget.pro.

#####################################################################
## tst_qquickwidget Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qquickwidget
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickwidget.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickWidgets
        Qt::QuickWidgetsPrivate
        Qt::WidgetsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickwidget.pro:<TRUE>:
# OTHER_FILES = "animating.qml" "error1.qml" "rectangle.qml" "resizemodeitem.qml"

## Scopes:
#####################################################################

qt_extend_target(tst_qquickwidget CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qquickwidget CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
