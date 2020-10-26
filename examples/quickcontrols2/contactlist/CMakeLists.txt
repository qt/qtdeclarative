# Generated from contactlist.pro.

cmake_minimum_required(VERSION 3.14)
project(contactlist LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols2/contactlist")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(contactlist
    contactmodel.cpp contactmodel.h
    main.cpp
)
set_target_properties(contactlist PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(contactlist PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
)


# Resources:
set(qmake_immediate_resource_files
    "ContactDelegate.ui.qml"
    "ContactDialog.qml"
    "ContactForm.ui.qml"
    "ContactView.ui.qml"
    "SectionDelegate.ui.qml"
    "contactlist.qml"
    "designer/Backend/ContactModel.qml"
)

qt6_add_resources(contactlist "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

install(TARGETS contactlist
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
