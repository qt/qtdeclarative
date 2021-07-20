# Generated from qquickmaterialstyle.pro.

#####################################################################
## tst_qquickmaterialstyle Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/tst_*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickmaterialstyle
    GUI
    QMLTEST
    SOURCES
        tst_qquickmaterialstyle.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
    TESTDATA ${test_data}
)

# Resources:
set(qmake_immediate_resource_files
    "qtquickcontrols2.conf"
)

qt_internal_add_resource(tst_qquickmaterialstyle "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:qquickmaterialstyle.pro:<TRUE>:
# OTHER_FILES = "$$PWD/data/*.qml"
# TEMPLATE = "app"
