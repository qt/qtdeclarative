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
if(TARGET Qt::Gui AND (QT_FEATURE_opengl OR QT_FEATURE_opengles2 OR QT_FEATURE_opengles3))
    add_subdirectory(particles)
endif()
