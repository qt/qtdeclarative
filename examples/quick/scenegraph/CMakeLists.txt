# Generated from scenegraph.pro.

add_subdirectory(customgeometry)
# add_subdirectory(rendernode) # special case needs fixing
add_subdirectory(threadedanimation)
if(QT_FEATURE_opengl OR QT_FEATURE_opengles2 OR QT_FEATURE_opengles3)
    add_subdirectory(graph)
    add_subdirectory(simplematerial)
    add_subdirectory(sgengine)
    add_subdirectory(fboitem)
    add_subdirectory(openglunderqml)
    add_subdirectory(textureinthread)
    add_subdirectory(twotextureproviders)
endif()
if(MACOS)
    # add_subdirectory(metalunderqml) # special case needs fixing
    # add_subdirectory(metaltextureimport) # special case TODO
endif()
if(WIN32)
    add_subdirectory(d3d11underqml)
endif()
if(QT_FEATURE_vulkan)
    #add_subdirectory(vulkanunderqml) # special case not yet converted
endif()
