add_subdirectory(unit)
add_subdirectory(app_subdir)

# These exercise the scanner-driven build-dir qt.conf, which is only generated for
# shared Qt builds (the qmlimportscanner deploy finalizer is shared-only).
if(QT6_IS_SHARED_LIBS_BUILD)
    add_subdirectory(qml_only_import)
    add_subdirectory(non_bundle)
endif()
