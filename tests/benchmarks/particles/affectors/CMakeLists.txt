# Generated from affectors.pro.

#####################################################################
## tst_affectors Binary:
#####################################################################

add_qt_executable(tst_affectors
    GUI
    SOURCES
        tst_affectors.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    LIBRARIES
        Qt::QuickParticlesPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::QuickParticles
        Qt::Test
)

#### Keys ignored in scope 1:.:.:affectors.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:affectors.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
