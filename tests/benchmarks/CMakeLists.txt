# Generated from benchmarks.pro.

add_subdirectory(qml)
add_subdirectory(quick)
if(QT_FEATURE_private_tests AND (QT_FEATURE_opengl OR QT_FEATURE_opengles2 OR QT_FEATURE_opengles3))
    add_subdirectory(particles)
endif()
