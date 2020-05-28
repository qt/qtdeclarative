# Generated from rendercontrol.pro.

add_subdirectory(rendercontrol_opengl)
if(WIN32 AND NOT MINGW)
    # add_subdirectory(rendercontrol_d3d11) # special case to be ported
endif()
