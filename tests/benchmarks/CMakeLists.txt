# Generated from benchmarks.pro.

add_subdirectory(qml)
add_subdirectory(quick)
if(QT_FEATURE_opengles2 OR QT_FEATURE_opengles3 OR (QT_FEATURE_opengl AND QT_FEATURE_private_tests))
    add_subdirectory(particles)
endif()
