qt_add_library(cppauto_test_module STATIC)
qt_autogen_tools_initial_setup(cppauto_test_module)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

# A module that consists of C++ types only. The types are exported in version 2.0, but the module
# itself is registered in version 2.7. The engine therefore resolves "import CppAuto" to 2.7 and
# passes that on to the "import CppAutoTarget auto" in the qmldir.
qt_add_qml_module(cppauto_test_module
    URI CppAuto
    VERSION 2.7
    SOURCES
        cppautotype.h
    IMPORTS
        CppAutoTarget/auto

    # Hide it at compile time from the TestTypes module. Only the consumer below gets to see it.
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/hidden/CppAuto"

    NO_LINT
    __QT_INTERNAL_NO_GENERATE_QMLLS_BUILD_INI
)

target_link_libraries(cppauto_test_module
    PRIVATE Qt6::Qml
)

qt_autogen_tools_initial_setup(cppauto_test_moduleplugin)

qt_add_library(cppauto_consumer_test_module STATIC)
qt_autogen_tools_initial_setup(cppauto_consumer_test_module)

set_target_properties(cppauto_consumer_test_module PROPERTIES
    # We really want qmlcachegen here, even if qmlsc is available
    QT_QMLCACHEGEN_EXECUTABLE qmlcachegen
)

qt_add_qml_module(cppauto_consumer_test_module
    URI CppAutoConsumer
    VERSION 1.0
    QML_FILES
        cppAuto.qml
        cppAuto2.qml
        cppAuto20.qml
        cppAuto27.qml
    IMPORT_PATH
        "${CMAKE_CURRENT_BINARY_DIR}/hidden"
        "${CMAKE_CURRENT_SOURCE_DIR}/../imports"
    NO_LINT
    __QT_INTERNAL_NO_GENERATE_QMLLS_BUILD_INI
)

add_dependencies(cppauto_consumer_test_module cppauto_test_module)

target_link_libraries(cppauto_consumer_test_module
    PRIVATE Qt6::Qml
)

qt_autogen_tools_initial_setup(cppauto_consumer_test_moduleplugin)
