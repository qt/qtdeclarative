# Generated from benchmarks.pro.

add_subdirectory(qml)
add_subdirectory(quick)
if(QT_FEATURE_private_tests)
    add_subdirectory(particles)
endif()
add_subdirectory(quickcontrols2)
