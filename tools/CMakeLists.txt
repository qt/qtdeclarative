# Generated from tools.pro.


if(QT_FEATURE_qml_devtools)
    add_subdirectory(qmllint)
    add_subdirectory(qmlimportscanner)
    # special case: Do not build qmlcachegen here but build it at src/
    # time, so that we can use it for our own .qml files in src/imports.
endif()

if(QT_FEATURE_thread AND NOT ANDROID OR android_app AND NOT WASM AND NOT rtems)
    add_subdirectory(qml)

    if(QT_FEATURE_qml_profiler)
        add_subdirectory(qmlprofiler)
    endif()

    if(QT_FEATURE_qml_preview)
        add_subdirectory(qmlpreview)
    endif()

    if(TARGET Qt::Quick)

        if(BUILD_SHARED_LIBS) # special case
            add_subdirectory(qmlscene)
            add_subdirectory(qmltime)

            if(QT_FEATURE_regularexpression AND QT_FEATURE_process)
                add_subdirectory(qmlplugindump)
            endif()
        endif()

        if(TARGET Qt::Widgets)

            if(QT_FEATURE_dialogbuttonbox)
                add_subdirectory(qmleasing)
            endif()
        endif()
    endif()

    if(TARGET Qt::QuickTest)
        add_subdirectory(qmltestrunner)
    endif()

    if(QT_FEATURE_private_tests)
        add_subdirectory(qmljs)
    endif()
endif()

if(QT_FEATURE_qml_devtools)
endif()

