find_package(Qt6 6.8 COMPONENTS Quick REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(app app.cpp)
qt_add_qml_module(app
    URI ExampleProjectApp
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES
        TARGET FooBar
)

target_link_libraries(app
    PRIVATE
        Qt6::Quick
        FooBarplugin
    )
