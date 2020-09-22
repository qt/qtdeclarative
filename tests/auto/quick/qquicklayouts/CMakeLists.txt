# Generated from qquicklayouts.pro.

#####################################################################
## tst_qquicklayouts Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquicklayouts
    GUI
    QMLTEST
    SOURCES
        tst_qquicklayouts.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquicklayouts.pro:<TRUE>:
# OTHER_FILES = "data/tst_rowlayout.qml" "data/tst_gridlayout.qml" "data/tst_stacklayout.qml"
# TEMPLATE = "app"
