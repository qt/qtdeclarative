qt_internal_add_test(tst_qmlbasicapp
    SOURCES
        tst_qmlbasicapp.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
)

qt6_add_qml_module(tst_qmlbasicapp
    VERSION 1.0
    URI "BasicApp"
    QML_FILES
        main.qml
)

add_subdirectory(BasicExtension)
add_subdirectory(TimeExample)

if(QT6_IS_SHARED_LIBS_BUILD)
    # Plugins are static, but qt6_import_qml_plugins() can't be used because
    # it does nothing for shared builds. Import and link the plugins manually.
    target_sources(tst_qmlbasicapp PRIVATE manual_imports.cpp)
    target_link_libraries(tst_qmlbasicapp PRIVATE
        additional_qml_module_plugin
        qmlqtimeexampleplugin
    )
else()
    # For fully static builds, let qmlimportscanner take care of everything
    qt6_import_qml_plugins(tst_qmlbasicapp)
endif()
