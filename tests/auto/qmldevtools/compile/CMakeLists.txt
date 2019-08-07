# Generated from compile.pro.

#####################################################################
## tst_compile Binary:
#####################################################################

add_qt_executable(tst_compile
    GUI
    SOURCES
        tst_compile.cpp
    LIBRARIES
        Qt::QmlDevToolsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::QmlDevTools
)

#### Keys ignored in scope 1:.:.:compile.pro:<TRUE>:
# _LOADED = "qt_common"
# _OPTION = "host_build"

## Scopes:
#####################################################################

extend_target(tst_compile CONDITION force_bootstrap
    LIBRARIES
        Qt::BootstrapPrivate
    PUBLIC_LIBRARIES
        Qt::Bootstrap
)

#### Keys ignored in scope 3:.:.:compile.pro:NOT build_pass:
# CONFIG = "release"

extend_target(tst_compile CONDITION NOT force_bootstrap
    PUBLIC_LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 5:.:.:compile.pro:NOT build_pass AND QT_FEATURE_debug_and_release:
# CONFIG = "release"

#### Keys ignored in scope 6:.:.:compile.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
