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

set(INSTALL_SHAREDDIR "${INSTALL_EXAMPLESDIR}/quick/${PROJECT_NAME}/shared")
install(TARGETS ${PROJECT_NAME}_shared
    RUNTIME DESTINATION "${INSTALL_SHAREDDIR}"
    LIBRARY DESTINATION "${INSTALL_SHAREDDIR}"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmldir
    DESTINATION "${INSTALL_SHAREDDIR}"
)
