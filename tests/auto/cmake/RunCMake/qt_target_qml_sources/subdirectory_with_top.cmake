find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Test)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(main)

qt_add_qml_module(main
    URI MyAppUri
    QML_FILES
        Main.qml
)

target_link_libraries(main
    PRIVATE
    Qt::Quick
    Qt::Test
)

add_subdirectory(src)
