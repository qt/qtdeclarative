# Generated from listmodel.pro.

#####################################################################
## tableview_listmodel Binary:
#####################################################################

add_qt_executable(tableview_listmodel
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
add_qt_resource(tableview_listmodel "qmake_immediate"
    PREFIX
        "/"
    FILES
        main.qml
)


#### Keys ignored in scope 1:.:.:listmodel.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:listmodel.pro:QNX:
# target.path = "/tmp/$${TARGET}/bin"

#### Keys ignored in scope 4:.:.:listmodel.pro:UNIX AND NOT ANDROID:
# target.path = "/opt/$${TARGET}/bin"

#### Keys ignored in scope 5:.:.:listmodel.pro:NOT target.path_ISEMPTY:
# INSTALLS = "target"
