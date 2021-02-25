# Generated from systemtrayicon.pro.

#####################################################################
## systemtrayicon Binary:
#####################################################################

qt_internal_add_manual_test(systemtrayicon
    GUI
    SOURCES
        systemtrayicon.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::QuickControls2
)

# Resources:
set(systemtrayicon_resource_files
    "images/qt_logo_green_256.png"
    "systemtrayicon.qml"
)

qt_internal_add_resource(systemtrayicon "systemtrayicon"
    PREFIX
        "/files"
    FILES
        ${systemtrayicon_resource_files}
)


#### Keys ignored in scope 1:.:.:systemtrayicon.pro:<TRUE>:
# TEMPLATE = "app"
