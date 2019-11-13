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
# _OPTION = "host_build"

## Scopes:
#####################################################################

extend_target(tst_compile CONDITION force_bootstrap
    LIBRARIES
        Qt::BootstrapPrivate
    PUBLIC_LIBRARIES
        Qt::Bootstrap
)

extend_target(tst_compile CONDITION NOT force_bootstrap
    PUBLIC_LIBRARIES
        Qt::Core
)
