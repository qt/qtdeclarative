# Generated from builtins.pro.

# special case begin

set(qml_type_file builtins.qmltypes)

qt_path_join(qml_install_dir "${QT_INSTALL_DIR}" "${INSTALL_QMLDIR}")
qt_copy_or_install(FILES ${qml_type_file}
    DESTINATION ${qml_install_dir}
)

# special case end
