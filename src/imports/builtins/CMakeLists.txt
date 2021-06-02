set(qml_type_files builtins.qmltypes jsroot.qmltypes)

qt_path_join(qml_install_dir "${QT_INSTALL_DIR}" "${INSTALL_QMLDIR}")
qt_copy_or_install(FILES ${qml_type_files}
    DESTINATION ${qml_install_dir}
)
