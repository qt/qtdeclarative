# Generated from styles.pro.

#####################################################################
## styles Binary:
#####################################################################

qt_internal_add_manual_test(styles
    GUI
    SOURCES
        styles.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::QuickControls2
)

# Resources:
set(qmake_immediate_resource_files
    "styles.qml"
)

qt_internal_add_resource(styles "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:styles.pro:<TRUE>:
# TEMPLATE = "app"
