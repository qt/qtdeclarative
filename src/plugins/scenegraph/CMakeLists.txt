# Generated from scenegraph.pro.

# special case begin
#if(QT_FEATURE_d3d12)
#    add_subdirectory(d3d12)
#endif()
# special case end

if(QT_FEATURE_openvg)
    add_subdirectory(openvg)
endif()
