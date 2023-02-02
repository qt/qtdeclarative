# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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

include(QtBundleQmlModuleForMacOS.cmake)
# Puts the shared qml module plugin and qmldir into the macOS app bundle directory.
# Only call this function if your main project has the MACOSX_BUNDLE option set.
function(bundle_shared app_target)
    set(qml_plugin_target "${PROJECT_NAME}_shared")
    set(qml_module_uri "shared")
    add_qml_module_to_macos_app_bundle(
        "${app_target}" "${qml_plugin_target}" "${qml_module_uri}")
endfunction()

set(INSTALL_SHAREDDIR "${INSTALL_EXAMPLESDIR}/quick/${PROJECT_NAME}/shared")
install(TARGETS ${PROJECT_NAME}_shared
    RUNTIME DESTINATION "${INSTALL_SHAREDDIR}"
    LIBRARY DESTINATION "${INSTALL_SHAREDDIR}"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmldir
    DESTINATION "${INSTALL_SHAREDDIR}"
)
