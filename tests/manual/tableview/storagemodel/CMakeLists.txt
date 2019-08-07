# Generated from storagemodel.pro.

#####################################################################
## tableview_storage Binary:
#####################################################################

add_qt_executable(tableview_storage
    GUI
    SOURCES
        main.cpp
        storagemodel.cpp storagemodel.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
add_qt_resource(tableview_storage "qmake_immediate"
    PREFIX
        "/"
    FILES
        main.qml
)


#### Keys ignored in scope 1:.:.:storagemodel.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:storagemodel.pro:QNX:
# target.path = "/tmp/$${TARGET}/bin"

#### Keys ignored in scope 4:.:.:storagemodel.pro:UNIX AND NOT ANDROID:
# target.path = "/opt/$${TARGET}/bin"

#### Keys ignored in scope 5:.:.:storagemodel.pro:NOT target.path_ISEMPTY:
# INSTALLS = "target"
