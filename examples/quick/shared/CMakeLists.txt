qt_add_library(${PROJECT_NAME}_shared)

set_source_files_properties(CheckBox.qml TabSet.qml TextField.qml
    PROPERTIES
        QT_QML_SOURCE_VERSIONS 2.1
)

set_source_files_properties(Images.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE true
        QT_QML_SOURCE_VERSIONS 2.2
)

qt_add_qml_module(${PROJECT_NAME}_shared
    URI shared
    VERSION 2.2
    PLUGIN_TARGET ${PROJECT_NAME}_shared
    SOURCES
        "shared.h"
    QML_FILES
        "Button.qml"
        "CheckBox.qml"
        "FlickrRssModel.qml"
        "Label.qml"
        "LauncherList.qml"
        "SimpleLauncherDelegate.qml"
        "Slider.qml"
        "TabSet.qml"
        "TextField.qml"
        "Images.qml"
    RESOURCES
        "images/back.png"
        "images/checkmark.png"
        "images/next.png"
        "images/qt-logo.png"
        "images/slider_handle.png"
        "images/tab.png"
)

# put the shared module into the macOS bundle directory.
# Only call this function if your main project is has the MACOSX_BUNDLE option set.
function(bundle_shared example)
    if(QT6_IS_SHARED_LIBS_BUILD AND APPLE)
        # The application's main.cpp adds an explicit QML import path to look for qml modules under
        # a PlugIns subdirectory in a macOS bundle.
        # Copy the qmldir and shared library qml plugin.

        # Ensure the executable depends on the plugin so the plugin is copied
        # only after it was built.
        add_dependencies(${example} ${PROJECT_NAME}_shared)

        set(shared_dir "$<TARGET_FILE_DIR:${PROJECT_NAME}_shared>")
        set(shared_qmldir_file "${shared_dir}/qmldir")
        set(app_dir "$<TARGET_FILE_DIR:${example}>")
        set(bundle_shared_dir "${app_dir}/../PlugIns/shared")

        add_custom_command(TARGET ${example} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${bundle_shared_dir}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:${PROJECT_NAME}_shared> ${bundle_shared_dir}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${shared_qmldir_file} ${bundle_shared_dir}
        )
    endif()
endfunction()

set(INSTALL_SHAREDDIR "${INSTALL_EXAMPLESDIR}/quick/${PROJECT_NAME}/shared")
install(TARGETS ${PROJECT_NAME}_shared
    RUNTIME DESTINATION "${INSTALL_SHAREDDIR}"
    LIBRARY DESTINATION "${INSTALL_SHAREDDIR}"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmldir
    DESTINATION "${INSTALL_SHAREDDIR}"
)
