# Generated from storagemodel.pro.

#####################################################################
## tableview_storage Binary:
#####################################################################

add_qt_manual_test(tableview_storage
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
set(qmake_immediate_resource_files
    "main.qml"
)

add_qt_resource(tableview_storage "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
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
