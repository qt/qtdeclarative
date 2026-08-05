qt_add_library(with_qml_files STATIC)
qt_add_qml_module(with_qml_files
    URI WithQml
    QML_FILES Main.qml
)
