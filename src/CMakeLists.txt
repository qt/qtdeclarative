# Generated from src.pro.

add_subdirectory(qml)
add_subdirectory(qmlmodels)
add_subdirectory(plugins)
#add_subdirectory(imports)

if(QT_FEATURE_qml_worker_script)
    add_subdirectory(qmlworkerscript)
endif()

if(TARGET Qt::Gui AND QT_FEATURE_qml_animation)
    add_subdirectory(quick)
    add_subdirectory(quickshapes)

    if(QT_FEATURE_testlib AND TARGET Qt::Test) # special case
        add_subdirectory(qmltest)
    endif()

    if(QT_FEATURE_quick_particles)
        add_subdirectory(particles)
    endif()

    if(TARGET Qt::Widgets)
        add_subdirectory(quickwidgets)
    endif()
endif()

if(QT_FEATURE_qml_devtools)
    add_subdirectory(qmldevtools)
endif()

if(QT_FEATURE_qml_network)

    if(QT_FEATURE_thread AND QT_FEATURE_localserver AND QT_FEATURE_qml_debug)
        add_subdirectory(qmldebug)
    endif()
endif()
