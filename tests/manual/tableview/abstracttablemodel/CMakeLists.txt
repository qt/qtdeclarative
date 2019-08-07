# Generated from abstracttablemodel.pro.

#####################################################################
## tableview_abstracttablemodel Binary:
#####################################################################

add_qt_executable(tableview_abstracttablemodel
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
add_qt_resource(tableview_abstracttablemodel "qmake_immediate"
    PREFIX
        "/"
    FILES
        Button.qml
        main.qml
)


#### Keys ignored in scope 1:.:.:abstracttablemodel.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:abstracttablemodel.pro:QNX:
# target.path = "/tmp/$${TARGET}/bin"

#### Keys ignored in scope 4:.:.:abstracttablemodel.pro:UNIX AND NOT ANDROID:
# target.path = "/opt/$${TARGET}/bin"

#### Keys ignored in scope 5:.:.:abstracttablemodel.pro:NOT target.path_ISEMPTY:
# INSTALLS = "target"
