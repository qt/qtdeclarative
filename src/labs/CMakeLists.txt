add_subdirectory(settings)
add_subdirectory(models)

if(QT_FEATURE_qml_itemmodel)
    add_subdirectory(folderlistmodel)
endif()

if(TARGET Qt::Quick)
    add_subdirectory(animation)
endif()

if(QT_FEATURE_quick_shadereffect AND TARGET Qt::Quick)
    add_subdirectory(wavefrontmesh)
endif()

if(QT_FEATURE_systemsemaphore AND TARGET Qt::Quick)
    add_subdirectory(sharedimage)
endif()
