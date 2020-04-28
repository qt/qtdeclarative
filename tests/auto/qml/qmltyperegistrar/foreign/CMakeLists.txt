# Generated from foreign.pro.

#####################################################################
## foreign Generic Library:
#####################################################################

qt_add_cmake_library(foreign
    STATIC
    SOURCES
        foreign.cpp foreign.h
    PUBLIC_LIBRARIES
        Qt::Core
)
set_property(TARGET foreign PROPERTY AUTOMOC ON)
qt6_generate_meta_types_json_file(foreign)

#### Keys ignored in scope 1:.:.:foreign.pro:<TRUE>:
# TEMPLATE = "lib"

## Scopes:
#####################################################################
