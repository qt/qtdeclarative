cmake_minimum_required(VERSION 3.16)
project(threading LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/threading")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../shared" "shared")

qt_add_executable(threadingexample
    main.cpp
)

set_target_properties(threadingexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(threadingexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

add_dependencies(threadingexample threading_shared)

qt_add_qml_module(threadingexample
    URI threading
    VERSION 1.0
    QML_FILES
        "threadedlistmodel/dataloader.mjs"
        "threadedlistmodel/timedisplay.qml"
        "threading.qml"
        "workerscript/Spinner.qml"
        "workerscript/workerscript.mjs"
        "workerscript/workerscript.qml"
)

install(TARGETS threadingexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(threadingexample)
