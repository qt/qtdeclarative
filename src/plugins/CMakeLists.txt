# Generated from plugins.pro.


if(QT_FEATURE_thread AND QT_FEATURE_qml_debug)
    add_subdirectory(qmltooling)
endif()

if(TARGET Qt::Quick)
    add_subdirectory(scenegraph)
endif()
