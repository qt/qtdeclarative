cmake_minimum_required(VERSION 3.18)
project(todolist LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/todolist")

find_package(Qt6 COMPONENTS Gui Qml Quick QuickControls2)

qt_add_executable(todolist
    src/main.cpp
)

set_target_properties(todolist PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(todolist PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::QuickControls2
)

set_source_files_properties(AppSettings.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

set_source_files_properties(Database.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(todolist
    URI App
    QML_FILES
        AppSettings.qml
        Database.qml
        FontSizePage.qml
        HomePage.qml
        MaxTasksPage.qml
        NavBar.qml
        ProjectPage.qml
        SettingsPage.qml
        ToggleCompletedTasksPage.qml
        main.qml
    RESOURCES
        images/back.png
        images/back@2x.png
        images/back@3x.png
        images/back-white.png
        images/back-white@2x.png
        images/back-white@3x.png
        images/close.png
        images/close@2x.png
        images/close@3x.png
        images/close-white.png
        images/close-white@2x.png
        images/close-white@3x.png
        images/plus-math.png
        images/plus-math@2x.png
        images/plus-math@3x.png
        images/settings.png
        images/settings@2x.png
        images/settings@3x.png
        images/add-new.png
        images/add-new@2x.png
        images/add-new@3x.png
    NO_RESOURCE_TARGET_PATH
)

set_property(GLOBAL PROPERTY XCODE_EMIT_EFFECTIVE_PLATFORM_NAME ON)

install(TARGETS todolist
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
