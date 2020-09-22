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
set_source_files_properties("../data/shared/shaders/+qsb/basic.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/basic.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/basic_alpha.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/basic_alpha.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/cmyk.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/cmyk.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/culling.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/culling.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/desaturate.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/desaturate.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/edge.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/edge.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/gradient.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/gradient.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/gradient2.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/gradient2.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/gradient3.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/gradient3.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/gradient4.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/gradient4.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/gradient5.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/gradient5.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/gradientgrid.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/gradientgrid.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/property.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/property.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/shadow.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/shadow.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/stencil.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/stencil.frag"
)
set_source_files_properties("../data/shared/shaders/+qsb/subrect.vert"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/subrect.vert"
)
set_source_files_properties("../data/shared/shaders/+qsb/wave.vert"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/+qsb/wave.vert"
)
set_source_files_properties("../data/shared/shaders/basic.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/basic.frag"
)
set_source_files_properties("../data/shared/shaders/basic_alpha.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/basic_alpha.frag"
)
set_source_files_properties("../data/shared/shaders/cmyk.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/cmyk.frag"
)
set_source_files_properties("../data/shared/shaders/culling.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/culling.frag"
)
set_source_files_properties("../data/shared/shaders/desaturate.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/desaturate.frag"
)
set_source_files_properties("../data/shared/shaders/edge.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/edge.frag"
)
set_source_files_properties("../data/shared/shaders/gradient.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/gradient.frag"
)
set_source_files_properties("../data/shared/shaders/gradient2.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/gradient2.frag"
)
set_source_files_properties("../data/shared/shaders/gradient3.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/gradient3.frag"
)
set_source_files_properties("../data/shared/shaders/gradient4.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/gradient4.frag"
)
set_source_files_properties("../data/shared/shaders/gradient5.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/gradient5.frag"
)
set_source_files_properties("../data/shared/shaders/gradientgrid.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/gradientgrid.frag"
)
set_source_files_properties("../data/shared/shaders/property.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/property.frag"
)
set_source_files_properties("../data/shared/shaders/shadow.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/shadow.frag"
)
set_source_files_properties("../data/shared/shaders/stencil.frag"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/stencil.frag"
)
set_source_files_properties("../data/shared/shaders/subrect.vert"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/subrect.vert"
)
set_source_files_properties("../data/shared/shaders/wave.vert"
    PROPERTIES QT_RESOURCE_ALIAS "shaders/wave.vert"
)
set(shared_resource_files
    "shaders/+qsb/basic.frag"
    "shaders/+qsb/basic_alpha.frag"
    "shaders/+qsb/cmyk.frag"
    "shaders/+qsb/culling.frag"
    "shaders/+qsb/desaturate.frag"
    "shaders/+qsb/edge.frag"
    "shaders/+qsb/gradient.frag"
    "shaders/+qsb/gradient2.frag"
    "shaders/+qsb/gradient3.frag"
    "shaders/+qsb/gradient4.frag"
    "shaders/+qsb/gradient5.frag"
    "shaders/+qsb/gradientgrid.frag"
    "shaders/+qsb/property.frag"
    "shaders/+qsb/shadow.frag"
    "shaders/+qsb/stencil.frag"
    "shaders/+qsb/subrect.vert"
    "shaders/+qsb/wave.vert"
    "shaders/basic.frag"
    "shaders/basic_alpha.frag"
    "shaders/cmyk.frag"
    "shaders/culling.frag"
    "shaders/desaturate.frag"
    "shaders/edge.frag"
    "shaders/gradient.frag"
    "shaders/gradient2.frag"
    "shaders/gradient3.frag"
    "shaders/gradient4.frag"
    "shaders/gradient5.frag"
    "shaders/gradientgrid.frag"
    "shaders/property.frag"
    "shaders/shadow.frag"
    "shaders/stencil.frag"
    "shaders/subrect.vert"
    "shaders/wave.vert"
)

qt_internal_add_resource(qmlscenegrabber "shared"
    PREFIX
        "/"
    BASE
        "../data/shared"
    FILES
        ${shared_resource_files}
)

