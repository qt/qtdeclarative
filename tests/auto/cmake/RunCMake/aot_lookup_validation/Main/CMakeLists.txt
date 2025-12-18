qt_add_qml_module(main
    URI Main
    QML_FILES
        Main.qml
)
add_dependencies(main other)

target_link_libraries(main PRIVATE Qt6::Qml)
