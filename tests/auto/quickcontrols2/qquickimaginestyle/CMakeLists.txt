# Generated from qquickimaginestyle.pro.

#####################################################################
## tst_qquickimaginestyle Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/tst_*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickimaginestyle
    GUI
    QMLTEST
    SOURCES
        tst_qquickimaginestyle.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)

# Resources:
set(qmake_immediate_resource_files
    "control-assets/button-background.9.png"
    "qtquickcontrols2.conf"
    "test-assets/button-background-1.png"
    "test-assets/button-background-2.png"
    "test-assets/wobble.frag.qsb"
)

qt_internal_add_resource(tst_qquickimaginestyle "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:qquickimaginestyle.pro:<TRUE>:
# OTHER_FILES = "$$PWD/data/*.qml"
# TEMPLATE = "app"
