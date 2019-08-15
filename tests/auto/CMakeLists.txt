# Generated from auto.pro.

add_subdirectory(qml)
add_subdirectory(quick)
add_subdirectory(quicktest)
add_subdirectory(qmldevtools)
add_subdirectory(cmake)
# special case begin
# add_subdirectory(installed_cmake)
# special case end
add_subdirectory(toolsupport)
if(NOT APPLE_UIKIT)
    add_subdirectory(qmltest)
endif()
if(QT_FEATURE_opengles2 OR QT_FEATURE_opengles3 OR (QT_FEATURE_opengl AND TARGET Qt::Gui))
    add_subdirectory(particles)
endif()
if(QT_FEATURE_opengles2 OR (QT_FEATURE_opengl AND TARGET Qt::Gui) OR (QT_FEATURE_opengles3 AND TARGET Qt::Widgets))
    add_subdirectory(quickwidgets)
endif()
