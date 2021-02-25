# Generated from buttons.pro.

#####################################################################
## buttons Binary:
#####################################################################

qt_internal_add_manual_test(buttons
    GUI
    SOURCES
        buttons.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QuickControls2
)

file(GLOB resource_glob_0 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.qml")
foreach(file IN LISTS resource_glob_0)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

# Resources:
set(qmake_immediate_resource_files
    ${resource_glob_0}
)

qt_internal_add_resource(buttons "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:buttons.pro:<TRUE>:
# TEMPLATE = "app"
