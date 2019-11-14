# Generated from qquickopenglinfo.pro.

#####################################################################
## tst_qquickopenglinfo Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qquickopenglinfo
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickopenglinfo.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickopenglinfo.pro:<TRUE>:
# OTHER_FILES = "data/basic.qml"

## Scopes:
#####################################################################

qt_extend_target(tst_qquickopenglinfo CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qquickopenglinfo CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
