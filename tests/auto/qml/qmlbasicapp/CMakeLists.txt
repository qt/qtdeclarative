# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qmlbasicapp
    SOURCES
        tst_qmlbasicapp.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
)

qt6_add_qml_module(tst_qmlbasicapp
    URI "BasicApp"
    RESOURCE_PREFIX "/"
    QML_FILES
        main.qml
)

add_subdirectory(BasicExtension)
add_subdirectory(TimeExample2)

if(QT6_IS_SHARED_LIBS_BUILD)
    # Plugins are static, but qt6_import_qml_plugins() can't be used because
    # it does nothing for shared builds. Import and link the plugins manually.
    target_sources(tst_qmlbasicapp PRIVATE manual_imports.cpp)
    target_link_libraries(tst_qmlbasicapp PRIVATE
        additional_qml_module_plugin
        qmlqtimeexample2plugin
    )
else()
    # For fully static builds, let qmlimportscanner take care of everything
    qt6_import_qml_plugins(tst_qmlbasicapp)
endif()
