# Generated from testapp.pro.

#####################################################################
## testapp Binary:
#####################################################################

add_qt_executable(testapp
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/./"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
)

# Resources:
add_qt_resource(testapp "main"
    PREFIX
        "/"
    FILES
        delayedExit.qml
        delayedQuit.qml
        immediateExit.qml
        immediateQuit.qml
)


#### Keys ignored in scope 1:.:.:testapp.pro:<TRUE>:
# CONFIG = "-app_bundle" "console"
# TEMPLATE = "app"
