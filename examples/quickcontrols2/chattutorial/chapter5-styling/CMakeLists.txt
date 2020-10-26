# Generated from chapter5-styling.pro.

cmake_minimum_required(VERSION 3.14)
project(chapter5-styling LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols2/chattutorial/chapter5-styling")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Sql)

qt_add_executable(chapter5-styling
    main.cpp
    sqlcontactmodel.cpp sqlcontactmodel.h
    sqlconversationmodel.cpp sqlconversationmodel.h
)
set_target_properties(chapter5-styling PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(chapter5-styling PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    Qt::Sql
)


# Resources:
set(qml_resource_files
    "+Material/ChatToolBar.qml"
    "ChatToolBar.qml"
    "ContactPage.qml"
    "ConversationPage.qml"
    "main.qml"
    "qtquickcontrols2.conf"
)

qt6_add_resources(chapter5-styling "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
set_source_files_properties("../shared/Albert_Einstein.png"
    PROPERTIES QT_RESOURCE_ALIAS "Albert_Einstein.png"
)
set_source_files_properties("../shared/Albert_Einstein@2x.png"
    PROPERTIES QT_RESOURCE_ALIAS "Albert_Einstein@2x.png"
)
set_source_files_properties("../shared/Albert_Einstein@3x.png"
    PROPERTIES QT_RESOURCE_ALIAS "Albert_Einstein@3x.png"
)
set_source_files_properties("../shared/Albert_Einstein@4x.png"
    PROPERTIES QT_RESOURCE_ALIAS "Albert_Einstein@4x.png"
)
set_source_files_properties("../shared/Ernest_Hemingway.png"
    PROPERTIES QT_RESOURCE_ALIAS "Ernest_Hemingway.png"
)
set_source_files_properties("../shared/Ernest_Hemingway@2x.png"
    PROPERTIES QT_RESOURCE_ALIAS "Ernest_Hemingway@2x.png"
)
set_source_files_properties("../shared/Ernest_Hemingway@3x.png"
    PROPERTIES QT_RESOURCE_ALIAS "Ernest_Hemingway@3x.png"
)
set_source_files_properties("../shared/Ernest_Hemingway@4x.png"
    PROPERTIES QT_RESOURCE_ALIAS "Ernest_Hemingway@4x.png"
)
set_source_files_properties("../shared/Hans_Gude.png"
    PROPERTIES QT_RESOURCE_ALIAS "Hans_Gude.png"
)
set_source_files_properties("../shared/Hans_Gude@2x.png"
    PROPERTIES QT_RESOURCE_ALIAS "Hans_Gude@2x.png"
)
set_source_files_properties("../shared/Hans_Gude@3x.png"
    PROPERTIES QT_RESOURCE_ALIAS "Hans_Gude@3x.png"
)
set_source_files_properties("../shared/Hans_Gude@4x.png"
    PROPERTIES QT_RESOURCE_ALIAS "Hans_Gude@4x.png"
)
set(shared_resource_files
    "Albert_Einstein.png"
    "Albert_Einstein@2x.png"
    "Albert_Einstein@3x.png"
    "Albert_Einstein@4x.png"
    "Ernest_Hemingway.png"
    "Ernest_Hemingway@2x.png"
    "Ernest_Hemingway@3x.png"
    "Ernest_Hemingway@4x.png"
    "Hans_Gude.png"
    "Hans_Gude@2x.png"
    "Hans_Gude@3x.png"
    "Hans_Gude@4x.png"
)

qt6_add_resources(chapter5-styling "shared"
    PREFIX
        "/"
    BASE
        "../shared"
    FILES
        ${shared_resource_files}
)

install(TARGETS chapter5-styling
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
