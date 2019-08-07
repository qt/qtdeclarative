# Generated from form.pro.

#####################################################################
## form Binary:
#####################################################################

add_qt_executable(form
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
add_qt_resource(form "qmake_immediate"
    PREFIX
        "/"
    FILES
        RowForm.qml
        main.qml
)


#### Keys ignored in scope 1:.:.:form.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:form.pro:QNX:
# target.path = "/tmp/$${TARGET}/bin"

#### Keys ignored in scope 4:.:.:form.pro:UNIX AND NOT ANDROID:
# target.path = "/opt/$${TARGET}/bin"

#### Keys ignored in scope 5:.:.:form.pro:NOT target.path_ISEMPTY:
# INSTALLS = "target"
