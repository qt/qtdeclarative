# Generated from rendercontrol.pro.

add_subdirectory(rendercontrol_opengl)
if(WIN32)
    add_subdirectory(rendercontrol_d3d11)
endif()
