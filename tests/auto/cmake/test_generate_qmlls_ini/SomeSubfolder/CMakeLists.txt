qt_add_qml_module(ModuleA
    URI ModuleA
    VERSION 1.0
    QML_FILES Main.qml
    OUTPUT_DIRECTORY ./qml/Some/Sub/Folder/ModuleA
)

qt_add_qml_module(ModuleB
    URI ModuleB
    VERSION 1.0
    QML_FILES Main.qml
    OUTPUT_DIRECTORY ./qml/Some/Sub/Folder/ModuleB
)
