add_subdirectory(data)

qt_internal_add_test(tst_qmlcppcodegen
    SOURCES
        tst_qmlcppcodegen.cpp
    LIBRARIES
        Qt::QmlPrivate
        Qt::Gui
        codegen_test_module
        codegen_test_moduleplugin
)
