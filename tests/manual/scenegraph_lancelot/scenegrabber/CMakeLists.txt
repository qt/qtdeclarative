# Generated from scenegrabber.pro.

#####################################################################
## qmlscenegrabber Binary:
#####################################################################

qt_internal_add_manual_test(qmlscenegrabber
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.." # special case
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
)

# Resources:
set(shared_resource_files
    "../data/shared/shaders/+qsb/basic.frag"
    "../data/shared/shaders/+qsb/basic_alpha.frag"
    "../data/shared/shaders/+qsb/cmyk.frag"
    "../data/shared/shaders/+qsb/culling.frag"
    "../data/shared/shaders/+qsb/desaturate.frag"
    "../data/shared/shaders/+qsb/edge.frag"
    "../data/shared/shaders/+qsb/gradient.frag"
    "../data/shared/shaders/+qsb/gradient2.frag"
    "../data/shared/shaders/+qsb/gradient3.frag"
    "../data/shared/shaders/+qsb/gradient4.frag"
    "../data/shared/shaders/+qsb/gradient5.frag"
    "../data/shared/shaders/+qsb/gradientgrid.frag"
    "../data/shared/shaders/+qsb/property.frag"
    "../data/shared/shaders/+qsb/shadow.frag"
    "../data/shared/shaders/+qsb/stencil.frag"
    "../data/shared/shaders/+qsb/subrect.vert"
    "../data/shared/shaders/+qsb/wave.vert"
    "../data/shared/shaders/basic.frag"
    "../data/shared/shaders/basic_alpha.frag"
    "../data/shared/shaders/cmyk.frag"
    "../data/shared/shaders/culling.frag"
    "../data/shared/shaders/desaturate.frag"
    "../data/shared/shaders/edge.frag"
    "../data/shared/shaders/gradient.frag"
    "../data/shared/shaders/gradient2.frag"
    "../data/shared/shaders/gradient3.frag"
    "../data/shared/shaders/gradient4.frag"
    "../data/shared/shaders/gradient5.frag"
    "../data/shared/shaders/gradientgrid.frag"
    "../data/shared/shaders/property.frag"
    "../data/shared/shaders/shadow.frag"
    "../data/shared/shaders/stencil.frag"
    "../data/shared/shaders/subrect.vert"
    "../data/shared/shaders/wave.vert"
)

qt_internal_add_resource(qmlscenegrabber "shared"
    PREFIX
        "/"
    BASE
        "../data/shared"
    FILES
        ${shared_resource_files}
)

