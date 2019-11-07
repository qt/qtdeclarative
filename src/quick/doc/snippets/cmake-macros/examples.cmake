#! [qt5_import_qml_plugins]
find_package(Qt5 COMPONENTS Quick QmlImportScanner)
add_executable(myapp main.cpp)
target_link_libraries(myapp Qt5::Quick)
qt5_import_qml_plugins(myapp)
#! [qt5_import_qml_plugins]
