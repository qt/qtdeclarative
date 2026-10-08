qt_add_library(cpp_shadows_qml_test_module STATIC)
qt_autogen_tools_initial_setup(cpp_shadows_qml_test_module)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_qml_module(cpp_shadows_qml_test_module
    URI CppShadowsQml
    VERSION 1.0
    SOURCES
        shadowed.h
    QML_FILES
        Shadowed.qml

    # Both the "TestTypes" and the "verify" module need to see it, but the "verify" module doesn't
    # see the default output directory.
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../shared_imports/CppShadowsQml

    NO_LINT
    __QT_INTERNAL_NO_GENERATE_QMLLS_BUILD_INI
)

target_link_libraries(cpp_shadows_qml_test_module
    PRIVATE Qt6::Qml
)

qt_autogen_tools_initial_setup(cpp_shadows_qml_test_moduleplugin)
