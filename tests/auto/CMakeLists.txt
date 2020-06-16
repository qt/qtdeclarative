# Generated from auto.pro.

add_subdirectory(qml)
add_subdirectory(quick)
add_subdirectory(quicktest)
add_subdirectory(qmldevtools)
# special case begin
# add_subdirectory(cmake)
# add_subdirectory(installed_cmake)
# special case end
add_subdirectory(toolsupport)
if(NOT UIKIT)
    add_subdirectory(qmltest)
endif()
if(TARGET Qt::Gui)
    add_subdirectory(particles)
endif()
if(TARGET Qt::Widgets)
    add_subdirectory(quickwidgets)
endif()
