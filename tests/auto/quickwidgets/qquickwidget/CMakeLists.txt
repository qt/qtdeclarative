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
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickWidgetsPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickWidgets
        Qt::Widgets
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickwidget.pro:<TRUE>:
# OTHER_FILES = "animating.qml" "error1.qml" "rectangle.qml" "resizemodeitem.qml"

## Scopes:
#####################################################################

qt_extend_target(tst_qquickwidget CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qquickwidget CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
