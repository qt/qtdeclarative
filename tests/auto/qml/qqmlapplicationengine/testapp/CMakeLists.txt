# Generated from testapp.pro.

#####################################################################
## testapp Binary:
#####################################################################

qt_internal_add_executable(testapp
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
)

# Resources:
set(main_resource_files
    "delayedExit.qml"
    "delayedQuit.qml"
    "immediateExit.qml"
    "immediateQuit.qml"
)

qt_internal_add_resource(testapp "main"
    PREFIX
        "/"
    FILES
        ${main_resource_files}
)


#### Keys ignored in scope 1:.:.:testapp.pro:<TRUE>:
# TEMPLATE = "app"
