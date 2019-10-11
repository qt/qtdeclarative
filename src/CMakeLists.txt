# Generated from src.pro.
# special case skip regeneration
add_subdirectory(qml)
add_subdirectory(qmlmodels)

if(QT_FEATURE_qml_worker_script)
    add_subdirectory(qmlworkerscript)
endif()

if(TARGET Qt::Gui AND QT_FEATURE_qml_animation)
    add_subdirectory(quick)
    add_subdirectory(quickshapes)

    find_package(Qt6 ${PROJECT_VERSION} CONFIG COMPONENTS Test) # special case
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

# special case begin
# These need to be included here since they have dependencies on the modules
# specified above.
add_subdirectory(plugins)

if(QT_FEATURE_qml_devtools)
    add_subdirectory(qmldevtools)

    # Build qmlcachegen now, so that we can use it in src/imports.
    if(QT_FEATURE_commandlineparser AND QT_FEATURE_xmlstreamwriter)
        add_subdirectory(../tools/qmlcachegen qmlcachegen)
    endif()
endif()

add_subdirectory(imports)

if(QT_FEATURE_qml_network)

    if(QT_FEATURE_thread AND QT_FEATURE_localserver AND QT_FEATURE_qml_debug)
        add_subdirectory(qmldebug)
    endif()
endif()
# special case end
