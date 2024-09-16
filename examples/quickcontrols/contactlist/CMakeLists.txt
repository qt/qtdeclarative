# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(contactlist LANGUAGES CXX)


find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(contactlistexample WIN32 MACOSX_BUNDLE
    contactmodel.cpp contactmodel.h
    main.cpp
)

qt_add_qml_module(contactlistexample
    URI contactlist
    QML_FILES
        "ContactDelegate.ui.qml"
        "ContactDialog.qml"
        "ContactForm.ui.qml"
        "ContactView.ui.qml"
        "SectionDelegate.ui.qml"
        "ContactList.qml"
        "designer/Backend/ContactModel.qml"
)

target_link_libraries(contactlistexample PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

install(TARGETS contactlistexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET contactlistexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
