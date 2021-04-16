# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/tst_*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_core
    GUI
    QMLTEST
    SOURCES
        tst_core.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
    TESTDATA ${test_data}
)
