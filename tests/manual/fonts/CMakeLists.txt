# Generated from fonts.pro.

#####################################################################
## fonts Binary:
#####################################################################

qt_internal_add_manual_test(fonts
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qml_resource_files
    "main.qml"
)

qt_internal_add_resource(fonts "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:fonts.pro:<TRUE>:
# QML_IMPORT_PATH = <EMPTY>
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:deployment.pri:UNIX AND NOT ANDROID:
# INSTALLS = "target"

#### Keys ignored in scope 5:.:.:deployment.pri:QNX:
# target.path = "/tmp/$${TARGET}/bin"

#### Keys ignored in scope 6:.:.:deployment.pri:else:
# target.path = "/opt/$${TARGET}/bin"
