# Generated from parserstress.pro.

#####################################################################
## tst_parserstress Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    tests/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_parserstress
    SOURCES
        tst_parserstress.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
    TESTDATA ${test_data} "dummy_imports.qml"
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_parserstress)
endif()

## Scopes:
#####################################################################
